use std::time::Duration;
use yarrow_core::bins::bin_range;
use yarrow_core::fake::{FakeDevice, DEMO_IMAGE, DEMO_PAGE_SIZE};
use yarrow_core::{decode_image, Calibration, Client, Error};

fn client() -> Client<FakeDevice> {
    Client::new(FakeDevice::demo())
}

#[test]
fn info_and_ping() {
    let mut c = client();
    c.ping().unwrap();
    let i = c.info().unwrap();
    assert_eq!(i.version, "0.1.0");
    assert_eq!(i.pagesize, 1024);
    assert_eq!(i.uid, None);
    let mut d = FakeDevice::demo();
    d.uid = Some("ab12".into());
    d.lang = Some("tr".into());
    let i = Client::new(d).info().unwrap();
    assert_eq!(i.uid.as_deref(), Some("ab12"));
    assert_eq!(i.lang.as_deref(), Some("tr"));
}

#[test]
fn downloaded_log_is_the_device_image() {
    let mut c = client();
    let mut steps = 0;
    let (li, image) = c.download_log(|_, _| steps += 1).unwrap();
    assert_eq!(steps, li.pages);
    assert_eq!(image, DEMO_IMAGE);
    assert_eq!(decode_image(&image, li.pagesize as usize).len(), 19484);
}

#[test]
fn time_theme_and_calibration() {
    let mut c = client();
    c.set_time(1_800_000_000).unwrap();
    assert_eq!(c.time().unwrap(), 1_800_000_000);
    c.set_theme(7).unwrap();
    assert!(matches!(c.set_theme(99), Err(Error::Device(_))));
    let cal = Calibration { sens_x100: 9100, dead_us: 70, e_pct: 95, h_pct: 105 };
    c.set_cal(&cal).unwrap();
    assert_eq!(c.cal().unwrap(), cal);
    assert!(matches!(c.set_cal(&Calibration { sens_x100: 5, ..cal }), Err(Error::Device(m)) if m == "range"));
}

#[test]
fn events_are_listed() {
    let ev = client().events().unwrap();
    assert_eq!(ev.len(), 2);
    assert_eq!(ev[1].level, 2);
}

#[test]
fn live_lines_are_skipped_inside_replies() {
    let mut c = client();
    c.live_start().unwrap();
    // The device keeps streaming while we ask other things.
    let info = c.info().unwrap();
    assert_eq!(info.stage, 5);
    let (_, image) = c.download_log(|_, _| {}).unwrap();
    assert_eq!(image, DEMO_IMAGE);
    let s = c.next_live(Duration::from_millis(10)).unwrap().unwrap();
    assert!(s.cpm >= 6.0 && s.battery == 87);
    c.live_stop().unwrap();
}

#[test]
fn bins_of_the_demo_log() {
    let samples = decode_image(DEMO_IMAGE, DEMO_PAGE_SIZE);
    let (start, bins) = bin_range(&samples, 600, samples[0].time, samples.last().unwrap().time);
    assert_eq!(start % 600, 0);
    assert!(bins.iter().flatten().count() > 100);
    // Sum of complete bins can never exceed the sum of all samples.
    let total: u64 = samples.iter().map(|s| s.value as u64).sum();
    let in_bins: u64 = bins.iter().flatten().map(|&v| v as u64).sum();
    assert!(in_bins <= total);
    // Ten one-minute samples per complete bin: the mean of a quiet session is about ten times the sample mean.
    let mean: f64 = bins.iter().flatten().map(|&v| v as f64).sum::<f64>() / bins.iter().flatten().count() as f64;
    assert!(mean > 20.0 && mean < 3500.0, "{mean}");
    // Gaps between sessions are gaps in the bins.
    assert!(bins.iter().any(|b| b.is_none()));
}

#[test]
fn detector_trait_over_the_fake_device() {
    use yarrow_core::{Detector, YarrowDetector};
    let mut d: Box<dyn Detector> = Box::new(YarrowDetector::new(FakeDevice::demo()));
    let i = d.info().unwrap();
    assert_eq!((i.driver, i.sens_x100), ("yarrow", Some(8000)));
    assert_eq!(d.download_log(&mut |_, _| {}).unwrap().len(), 19484);
    d.live_start().unwrap();
    let r = d.next_live(Duration::from_millis(10)).unwrap().unwrap();
    // The field values are there for the screen, flagged as local only.
    assert_eq!(r.local_only.len(), 2);
    assert!(r.cpm > 0.0);
    d.live_stop().unwrap();
    assert!(d.tools().unwrap().cal().is_ok());
}
