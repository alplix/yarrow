//! Line transports: the real serial port and the trait the client is written against.

use crate::{Error, Result};
use serde::Serialize;
use std::io::{Read, Write};
use std::time::{Duration, Instant};

/// USB ids the firmware uses. These are test ids until the project gets its own.
pub const USB_VID: u16 = 0x1209;
pub const USB_PID: u16 = 0x0001;

/// A line-oriented connection to a device.
pub trait Transport: Send {
    fn send_line(&mut self, line: &str) -> Result<()>;
    /// The next line without its line ending, or `None` if nothing arrived within `timeout`.
    fn recv_line(&mut self, timeout: Duration) -> Result<Option<String>>;
}

/// A serial port found on this computer.
#[derive(Debug, Clone, Serialize)]
pub struct PortEntry {
    pub name: String,
    pub vid: Option<u16>,
    pub pid: Option<u16>,
    pub serial_number: Option<String>,
    pub product: Option<String>,
    /// True for a port with the Yarrow USB ids.
    pub is_yarrow: bool,
}

/// All serial ports, Yarrow devices first.
pub fn list_ports() -> Result<Vec<PortEntry>> {
    let mut v: Vec<PortEntry> = serialport::available_ports()?
        .into_iter()
        .map(|p| match p.port_type {
            serialport::SerialPortType::UsbPort(u) => PortEntry {
                is_yarrow: u.vid == USB_VID && u.pid == USB_PID,
                name: p.port_name,
                vid: Some(u.vid),
                pid: Some(u.pid),
                serial_number: u.serial_number,
                product: u.product,
            },
            _ => PortEntry {
                name: p.port_name,
                vid: None,
                pid: None,
                serial_number: None,
                product: None,
                is_yarrow: false,
            },
        })
        .collect();
    v.sort_by_key(|p| !p.is_yarrow);
    Ok(v)
}

/// The USB serial port of a real device.
pub struct SerialTransport {
    port: Box<dyn serialport::SerialPort>,
    buf: Vec<u8>,
}

impl SerialTransport {
    pub fn open(name: &str) -> Result<Self> {
        let mut port = serialport::new(name, 115_200).timeout(Duration::from_millis(50)).open()?;
        // CDC devices often wait for the host to raise DTR before they talk.
        let _ = port.write_data_terminal_ready(true);
        let _ = port.clear(serialport::ClearBuffer::All);
        Ok(SerialTransport { port, buf: Vec::new() })
    }
}

impl Transport for SerialTransport {
    fn send_line(&mut self, line: &str) -> Result<()> {
        self.port.write_all(line.as_bytes())?;
        self.port.write_all(b"\n")?;
        self.port.flush()?;
        Ok(())
    }

    fn recv_line(&mut self, timeout: Duration) -> Result<Option<String>> {
        let deadline = Instant::now() + timeout;
        loop {
            if let Some(i) = self.buf.iter().position(|&b| b == b'\n') {
                let line: Vec<u8> = self.buf.drain(..=i).collect();
                let s = String::from_utf8_lossy(&line).trim_end_matches(['\r', '\n']).to_string();
                return Ok(Some(s));
            }
            if Instant::now() >= deadline {
                return Ok(None);
            }
            let mut chunk = [0u8; 512];
            match self.port.read(&mut chunk) {
                Ok(0) => {}
                Ok(n) => self.buf.extend_from_slice(&chunk[..n]),
                Err(e) if e.kind() == std::io::ErrorKind::TimedOut => {}
                Err(e) => return Err(Error::Io(e)),
            }
        }
    }
}
