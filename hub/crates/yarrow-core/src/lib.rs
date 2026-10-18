//! Talks to a Yarrow-firmware radiation detector over its USB serial port and decodes the log it stores.
//!
//! * [`ylc`] and [`ylog`] decode the compressed log exactly as the firmware does.
//! * [`detector`] is the interface any Geiger counter driver implements; [`YarrowDetector`] is the first one.
//! * [`client`] speaks the text protocol over any [`transport::Transport`]: the real serial port or the
//!   in-memory [`fake::FakeDevice`] used by tests and the demo mode.
//! * [`bins`] cuts decoded samples into fixed-length count bins.

pub mod bins;
pub mod client;
pub mod detector;
pub mod fake;
pub mod id;
pub mod transport;
pub mod ylc;
pub mod ylog;

pub use client::{Calibration, Client, Event, Info, LiveSample, LogInfo};
pub use transport::{list_ports, PortEntry, SerialTransport, Transport};
pub use ylog::{decode_image, Sample};

/// Everything that can go wrong while talking to the device.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("serial port: {0}")]
    Io(#[from] std::io::Error),
    #[error("serial port: {0}")]
    Port(#[from] serialport::Error),
    #[error("the device did not answer in time")]
    Timeout,
    #[error("the device refused: {0}")]
    Device(String),
    #[error("unexpected reply: {0}")]
    Protocol(String),
}

pub type Result<T> = std::result::Result<T, Error>;

pub use detector::{Detector, DetectorInfo, LocalValue, Reading, Tools, YarrowDetector};
pub use id::{device_hash, fallback_id};
