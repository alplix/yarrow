//! BOINC account access through the standard web RPCs `lookup_account.php` and `create_account.php`.

use crate::{check_server_url, Error, Result};
use md5::{Digest, Md5};
use std::time::Duration;

/// What BOINC wants instead of a password: md5(password + lower-case email address).
pub fn passwd_hash(password: &str, email: &str) -> String {
    let mut h = Md5::new();
    h.update(password.as_bytes());
    h.update(email.trim().to_lowercase().as_bytes());
    hex::encode(h.finalize())
}

fn tag<'a>(xml: &'a str, name: &str) -> Option<&'a str> {
    let open = format!("<{name}>");
    let close = format!("</{name}>");
    let a = xml.find(&open)? + open.len();
    let b = xml[a..].find(&close)? + a;
    Some(xml[a..b].trim())
}

/// Reads the answer of either RPC: the account key, or the project's error message.
pub fn parse_account_reply(xml: &str) -> Result<String> {
    if let Some(key) = tag(xml, "authenticator") {
        if !key.is_empty() {
            return Ok(key.to_string());
        }
    }
    let msg = tag(xml, "error_msg").unwrap_or("").to_string();
    let num = tag(xml, "error_num").unwrap_or("");
    Err(Error::Server(match (msg.is_empty(), num) {
        (false, _) => msg,
        (true, "") => "the project did not return an account key".into(),
        (true, n) => format!("the project answered with error {n}"),
    }))
}

fn post(server: &str, script: &str, fields: &[(&str, &str)]) -> Result<String> {
    let server = check_server_url(server)?;
    let agent = ureq::AgentBuilder::new().timeout(Duration::from_secs(20)).build();
    // POST, so the password hash does not end up in server logs the way a query string would.
    match agent.post(&format!("{server}/{script}")).send_form(fields) {
        Ok(r) => r.into_string().map_err(|e| Error::Network(e.to_string())),
        Err(ureq::Error::Status(code, r)) => {
            // BOINC reports account errors in the body; fall back to the status line.
            let body = r.into_string().unwrap_or_default();
            match tag(&body, "error_msg") {
                Some(m) if !m.is_empty() => Err(Error::Server(m.to_string())),
                _ => Err(Error::Network(format!("the server answered HTTP {code}"))),
            }
        }
        Err(e) => Err(Error::Network(e.to_string())),
    }
}

/// Signs in with an existing account. Returns the account key (authenticator).
pub fn login(server: &str, email: &str, password: &str) -> Result<String> {
    let hash = passwd_hash(password, email);
    let xml = post(server, "lookup_account.php", &[("email_addr", email.trim()), ("passwd_hash", &hash)])?;
    parse_account_reply(&xml)
}

/// Creates an account. Returns the account key (authenticator).
pub fn create_account(server: &str, email: &str, password: &str, user_name: &str) -> Result<String> {
    if password.chars().count() < 6 {
        return Err(Error::Invalid("the password must have at least 6 characters".into()));
    }
    let hash = passwd_hash(password, email);
    let xml = post(
        server,
        "create_account.php",
        &[("email_addr", email.trim()), ("passwd_hash", &hash), ("user_name", user_name.trim())],
    )?;
    parse_account_reply(&xml)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hash_matches_the_boinc_rule() {
        // md5("secret" + "user@example.org")
        assert_eq!(passwd_hash("secret", "User@Example.org "), passwd_hash("secret", "user@example.org"));
        assert_eq!(passwd_hash("", "").len(), 32);
        assert_eq!(passwd_hash("", ""), "d41d8cd98f00b204e9800998ecf8427e");
    }

    #[test]
    fn replies_are_understood() {
        assert_eq!(parse_account_reply("<account_out>\n<authenticator>abc123</authenticator>\n</account_out>").unwrap(), "abc123");
        let e = parse_account_reply("<account_out><error_num>-136</error_num><error_msg>Bad password</error_msg></account_out>");
        assert!(matches!(e, Err(Error::Server(m)) if m == "Bad password"));
        let e = parse_account_reply("<error><error_num>-137</error_num></error>");
        assert!(matches!(e, Err(Error::Server(m)) if m.contains("-137")));
        assert!(parse_account_reply("garbage").is_err());
    }
}
