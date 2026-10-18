//! Runs the stand-in project server on 127.0.0.1:8787 with one account (demo@example.org / demo-password) so the Hub's
//! sharing page can be tried without the real endpoint.

fn main() {
    let s = yarrow_share::mock::MockServer::start_on("127.0.0.1:8787");
    s.add_account("demo@example.org", "demo-password", "demo-account-key");
    println!("mock project server at {}  (demo@example.org / demo-password)", s.url);
    loop {
        std::thread::sleep(std::time::Duration::from_secs(3600));
    }
}
