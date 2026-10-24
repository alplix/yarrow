"""Builds the Yarrow website (yarrow.athena.org.tr): index.html (English) and index.<lang>.html for the other languages, a
favicon, robots.txt and sitemap.xml, all plain static files in this directory.   Run:  python site/build.py

The text of every language is written once, in render(), through p(): p("English", "Türkçe"). More languages are added by
extending LANGS and the p() calls (or with the Tilvar translation API as the other Athena sites do).
"""
import html
import os
import shutil

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
BASE = "https://yarrow.athena.org.tr"
RELEASE = "https://github.com/alplix/yarrow/releases/latest/download/"
LANGS = [("en", "English", "en_US", "index.html"), ("tr", "Türkçe", "tr_TR", "index.tr.html")]

FIRMWARE = [("tr", "Türkçe"), ("en", "English"), ("de", "Deutsch"), ("fr", "Français"), ("es", "Español"), ("it", "Italiano"),
            ("pt", "Português"), ("nl", "Nederlands"), ("pl", "Polski"), ("ru", "Русский"), ("uk", "Українська"),
            ("id", "Bahasa Indonesia"), ("vi", "Tiếng Việt"), ("ja", "日本語"), ("ko", "한국어"), ("zh", "中文")]

PAL = {"p": "#ff77a8", "w": "#fff1e8", "l": "#00e436"}

# the flower as in the firmware (tools/gensprites.py flower()): petals pink, centres white, stem and leaves green
def flower_cells():
    w, h = 15, 18
    g = [["." for _ in range(w)] for _ in range(h)]
    centres = ((3, 3), (7, 2), (11, 3), (5, 6), (9, 6))
    for (cx, cy) in centres:
        for (dx, dy) in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            g[cy + dy][cx + dx] = "p"
    for (cx, cy) in centres:
        g[cy][cx] = "w"
    for y in range(8, 18):
        g[y][7] = "l"
    for (x, y) in ((6, 9), (5, 8), (8, 10), (9, 9), (6, 12), (5, 11), (4, 11), (8, 13), (9, 12), (10, 12),
                   (6, 15), (5, 14), (8, 16), (9, 15)):
        g[y][x] = "l"
    return g


def flower_svg(size=None, extra=""):
    g = flower_cells()
    rects = "".join(f'<rect x="{x}" y="{y}" width="1" height="1" fill="{PAL[c]}"/>'
                    for y, row in enumerate(g) for x, c in enumerate(row) if c != ".")
    dim = f' width="{size}" height="{int(size * 18 / 15)}"' if size else ""
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 15 18"{dim} shape-rendering="crispEdges"{extra}>{rects}</svg>'


LEGEND = [("#29adff", "< 80"), ("#00e436", "80 - 150"), ("#a16cdd", "150 - 300"), ("#ff77a8", "300 - 600"),
          ("#ff004d", "600 - 1500"), ("#fff1e8", "> 1500")]

CSS = """
:root{--bg:#0b1020;--bg2:#10172e;--panel:#141d3a;--panel2:#1a2547;--line:#2a3a6e;--text:#e8ecfb;--muted:#a8b3d6;--muted2:#7886b3;
--green:#00e436;--blue:#29adff;--violet:#a16cdd;--pink:#ff77a8;--red:#ff004d;--ink:#05081a}
*{box-sizing:border-box}html{scroll-behavior:smooth}
body{margin:0;background:linear-gradient(180deg,var(--bg),var(--bg2) 70%);color:var(--text);line-height:1.6;
font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
a{color:var(--blue)}.wrap{max-width:1120px;margin:0 auto;padding:0 22px}
.px{font-family:ui-monospace,"Cascadia Mono","SF Mono",Consolas,monospace;letter-spacing:.02em}
header.site{position:sticky;top:0;z-index:20;background:rgba(11,16,32,.9);backdrop-filter:blur(8px);border-bottom:3px solid var(--ink);
box-shadow:0 2px 0 var(--line)}
.bar{display:flex;align-items:center;gap:16px;padding:10px 0;flex-wrap:wrap}
.brand{display:flex;align-items:center;gap:10px;font-weight:800;font-size:1.2rem;color:var(--text);text-decoration:none}
nav.links{display:flex;gap:16px;flex:1;overflow-x:auto;white-space:nowrap;scrollbar-width:none;font-size:.9rem}
nav.links a{color:var(--muted);text-decoration:none}nav.links a:hover{color:var(--text)}
.lang a{color:var(--muted2);text-decoration:none;border:2px solid var(--line);padding:2px 8px;font-size:.78rem}
.lang a.cur{color:var(--green);border-color:var(--green)}
.btn{display:inline-block;padding:10px 20px;color:var(--ink);background:var(--green);border:3px solid var(--ink);font-weight:800;
text-decoration:none;box-shadow:4px 4px 0 var(--ink);transition:transform .08s}
.btn:hover{transform:translate(2px,2px);box-shadow:2px 2px 0 var(--ink)}
.btn.alt{background:var(--panel2);color:var(--text);border-color:var(--line);box-shadow:4px 4px 0 var(--ink)}
.hero{padding:56px 0 36px;text-align:center;position:relative;overflow:hidden}
.hero h1{font-size:clamp(2rem,5vw,3.3rem);margin:12px 0 10px;line-height:1.15}
.hero h1 b{color:var(--green)}.tag{color:var(--muted);font-size:1.15rem;max-width:760px;margin:0 auto 26px}
.cta{display:flex;gap:14px;justify-content:center;flex-wrap:wrap;margin-bottom:26px}
.pills{display:flex;gap:10px;justify-content:center;flex-wrap:wrap;margin-top:22px}
.pill{border:2px solid var(--line);background:var(--panel);padding:4px 12px;font-size:.82rem;color:var(--muted)}
.screens{max-width:1000px;margin:6px auto 0;border:3px solid var(--ink);box-shadow:inset 0 0 0 2px var(--line),6px 6px 0 var(--ink)}
.screens img{display:block;width:100%;height:auto;image-rendering:auto}
.pulses{display:flex;gap:6px;justify-content:center;margin:0 0 4px}.pulses i{width:10px;height:10px;background:var(--green);opacity:.15;animation:pulse 2.4s infinite}
@keyframes pulse{0%,60%,100%{opacity:.15}8%{opacity:1}}
.pulses i:nth-child(2){animation-delay:.5s}.pulses i:nth-child(3){animation-delay:1.1s}.pulses i:nth-child(4){animation-delay:.2s}
.pulses i:nth-child(5){animation-delay:1.7s}.pulses i:nth-child(6){animation-delay:.8s}.pulses i:nth-child(7){animation-delay:1.4s}
section{padding:54px 0}section.alt{background:rgba(255,255,255,.02);border-top:3px solid var(--ink);border-bottom:3px solid var(--ink)}
h2{font-size:1.7rem;margin:0 0 6px}h2 .px{color:var(--green)}.sub{color:var(--muted);max-width:780px;margin:0 0 26px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:16px}
.card{background:var(--panel);border:3px solid var(--ink);box-shadow:inset 0 0 0 2px var(--line),4px 4px 0 var(--ink);padding:16px 18px}
.card h3{margin:0 0 6px;font-size:1.05rem}.card p{margin:0;color:var(--muted);font-size:.95rem}.ic{font-size:1.6rem;display:block;margin-bottom:6px}
.steps{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px;counter-reset:s}
.step{background:var(--panel);border:3px solid var(--ink);box-shadow:inset 0 0 0 2px var(--line),4px 4px 0 var(--ink);padding:16px 18px;position:relative}
.step:before{counter-increment:s;content:counter(s);position:absolute;top:-14px;left:14px;background:var(--green);color:var(--ink);
font-weight:800;width:28px;height:28px;text-align:center;line-height:28px;border:3px solid var(--ink)}
.step h3{margin:6px 0 6px;font-size:1.05rem}.step p{margin:0;color:var(--muted);font-size:.94rem}
table{border-collapse:collapse;width:100%;font-size:.93rem}th,td{padding:9px 10px;border-bottom:2px solid var(--line);text-align:left;vertical-align:top}
th{color:var(--muted2);font-weight:600}
.tag-ok{color:var(--green);font-weight:700}.tag-wip{color:var(--blue);font-weight:700}
.dl{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px;margin:16px 0}
.dl a{display:block;text-align:center;padding:9px 6px;background:var(--panel2);border:3px solid var(--ink);color:var(--text);
text-decoration:none;box-shadow:3px 3px 0 var(--ink);font-size:.9rem}.dl a:hover{border-color:var(--green)}.dl small{display:block;color:var(--muted2)}
.legend{display:flex;flex-wrap:wrap;gap:10px;margin:14px 0}.legend span{display:flex;align-items:center;gap:6px;font-size:.88rem;color:var(--muted)}
.legend i{width:16px;height:16px;border:2px solid var(--ink);display:inline-block}
details{background:var(--panel);border:3px solid var(--ink);box-shadow:inset 0 0 0 2px var(--line);padding:10px 16px;margin:10px 0}
summary{cursor:pointer;font-weight:700}details p{color:var(--muted);margin:8px 0 2px}
ol.num{padding-left:22px}ol.num li{margin:8px 0}code{background:var(--ink);padding:1px 6px}
footer{border-top:3px solid var(--ink);padding:26px 0;color:var(--muted2);font-size:.88rem;text-align:center}
footer a{color:var(--muted)}
@media (max-width:640px){.lang{order:3}nav.links{order:4;width:100%}}
@media (prefers-reduced-motion:reduce){.pulses i{animation:none;opacity:.6}}
"""


def render(lang, name, locale, fname):
    def p(en, tr):
        return tr if lang == "tr" else en

    other = [(c, n, f) for (c, n, _l, f) in LANGS]
    title = p("Yarrow: a live map of natural background radiation", "Yarrow: doğal fon radyasyonunun canlı haritası")
    desc = p("Yarrow turns volunteers' Geiger counters into a live, open map of the natural background dose rate: open firmware for the FNIRSI GC-03, a BOINC monitor, statistical checks and a world map.",
             "Yarrow, gönüllülerin Geiger sayaçlarını doğal fon doz hızının canlı, açık bir haritasına dönüştürür: FNIRSI GC-03 için açık yazılım, BOINC monitörü, istatistiksel denetimler ve dünya haritası.")
    langsw = "".join(f'<a href="/{f if f != "index.html" else ""}" hreflang="{c}"{" class=cur" if c == lang else ""}>{n}</a>' for c, n, f in other)
    hreflang = "".join(f'<link rel="alternate" hreflang="{c}" href="{BASE}/{f if f != "index.html" else ""}"/>' for c, n, f in other)
    hreflang += f'<link rel="alternate" hreflang="x-default" href="{BASE}/"/>'
    self_url = BASE + "/" + (fname if fname != "index.html" else "")

    dl = "".join(f'<a href="{RELEASE}yarrow-gc03-{c}.bin" rel="noopener">{n}<small>yarrow-gc03-{c}.bin</small></a>' for c, n in FIRMWARE)
    legend = "".join(f'<span><i style="background:{c}"></i>{t}</span>' for c, t in LEGEND)

    feats = [
        ("☢️", p("Dose rate you can trust", "Güvenebileceğiniz doz hızı"),
         p("An adaptive averaging window: quiet at background level, quick when the level rises. Dead-time correction keeps strong sources honest.",
           "Uyarlanır ortalama penceresi: fon seviyesinde sakin, seviye yükselince hızlı. Ölü zaman düzeltmesi güçlü kaynakları doğru okutur.")),
        ("🚨", p("Alarms and an emergency guide", "Alarmlar ve acil durum rehberi"),
         p("Caution, danger and evacuate levels with hysteresis, sound, vibration and light, and a plain-language guide for what to do.",
           "Histerezisli dikkat, tehlike ve tahliye seviyeleri; ses, titreşim ve ışık; ne yapılacağını düz dille anlatan bir rehber.")),
        ("📒", p("A log that survives anything", "Her şeye dayanan kayıt"),
         p("From one-second detail for weeks to hourly records for years, power-fail safe, with alarm moments saved every second.",
           "Haftalarca saniye ayrıntısından yıllarca saatlik kayda; elektrik kesintisine dayanıklı, alarm anları saniyede bir saklanır.")),
        ("📊", p("Sessions, budget and sample test", "Oturumlar, bütçe ve numune testi"),
         p("Track a daily dose budget, record sessions and compare a sample against the background with a statistical verdict.",
           "Günlük doz bütçesi tutun, oturum kaydedin, bir numuneyi fon ile istatistiksel bir kararla karşılaştırın.")),
        ("⚡", p("Field and wire finder", "Alan ve kablo bulucu"),
         p("Electric and magnetic field readings with a rising tone and bars as you approach a source or a wire in the wall.",
           "Elektrik ve manyetik alan ölçümü; bir kaynağa ya da duvardaki kabloya yaklaşırken yükselen ses ve çubuklar.")),
        ("🎨", p("Themes, 16 languages, battery care", "Temalar, 16 dil, pil bakımı"),
         p("Sixteen colour themes, 16 languages, a battery gauge with health and charge cycles, and a self test of the tube, sensors and clock.",
           "On altı renk teması, 16 dil, sağlık ve şarj döngüsü gösteren pil göstergesi; tüp, algılayıcı ve saat için kendi kendini sınama.")),
        ("🔬", p("Calibration wizard", "Kalibrasyon sihirbazı"),
         p("Put the detector next to any source whose dose rate you know and it finds the sensitivity of your own tube (default: 180 counts per minute for 1 µSv/h).",
           "Dedektörü doz hızını bildiğiniz bir kaynağın yanına koyun; kendi tüpünüzün duyarlılığını bulur (varsayılan: 1 µSv/h için dakikada 180 sayım).")),
        ("🔌", p("A simple USB protocol", "Basit bir USB protokolü"),
         p("Raw pulse counter, identity, calibration and clock over a USB serial line, documented and open. This is what the monitor reads.",
           "Ham darbe sayacı, kimlik, kalibrasyon ve saat; USB seri hat üzerinden, belgelenmiş ve açık. Monitörün okuduğu budur.")),
    ]
    feat_html = "".join(f'<div class="card"><span class="ic">{i}</span><h3>{h}</h3><p>{b}</p></div>' for i, h, b in feats)

    trust = [
        (p("Physics at the door", "Kapıda fizik"), p("Impossible bins are refused at once: a dose rate over 1,000 µSv/h, data from the future or the distant past, an impossible dead time or sensitivity.",
                                                      "İmkânsız dilimler hemen reddedilir: 1.000 µSv/h üzeri doz hızı, gelecekten ya da çok eskiden gelen veri, imkânsız ölü zaman ya da duyarlılık.")),
        (p("The statistics test", "İstatistik testi"), p("A day later each stream is tested. Real counts are random, so their spread must match Poisson statistics. A made-up constant is too smooth and fails.",
                                                         "Bir gün sonra her akış sınanır. Gerçek sayımlar rastgeledir; dağılımları Poisson istatistiğine uymalıdır. Uydurma bir sabit fazla düzgündür ve geçemez.")),
        (p("Identity and limits", "Kimlik ve sınırlar"), p("Every detector has its own identity from its chip number. A copy of another detector's counts, a recorded day sent again or a dose rate far from the city's other detectors is held and looked at. New detectors earn a part of the credit that grows with a consistent history; up to 5 detectors per volunteer, a daily cap.",
                                                           "Her dedektörün çip numarasından türeyen kendi kimliği vardır. Başka bir dedektörün sayımlarının kopyası, yeniden gönderilen kayıtlı bir gün ya da şehrin diğer dedektörlerinden çok uzak bir doz hızı bekletilir ve incelenir. Yeni dedektörler, tutarlı bir geçmişle büyüyen bir kredi payı alır; gönüllü başına en fazla 5 dedektör, günlük üst sınır.")),
        (p("The crowd", "Kalabalık"), p("The map shows the median of a city, so one wrong detector cannot move it. Credit comes only after the checks.",
                                         "Harita bir şehrin medyanını gösterir; tek bir hatalı dedektör onu oynatamaz. Kredi yalnızca denetimlerden sonra verilir.")),
    ]
    trust_html = "".join(f'<div class="card"><h3>{h}</h3><p>{b}</p></div>' for h, b in trust)

    faq = [
        (p("Does it use my CPU?", "İşlemcimi kullanır mı?"), p("Practically not. The monitor reads a counter about once a second and sends a small message every ten minutes.",
                                                              "Neredeyse hayır. Monitör sayacı saniyede yaklaşık bir kez okur ve on dakikada bir küçük bir mesaj gönderir.")),
        (p("Where should I put the detector?", "Dedektörü nereye koymalıyım?"), p("Anywhere that stays the same, a shelf or a desk is fine. Move it rarely, the building and the surroundings are part of the reading.",
                                                                              "Hep aynı kalan bir yere; raf ya da masa olur. Nadiren yerini değiştirin, bina ve çevre ölçümün bir parçasıdır.")),
        (p("My value differs from my neighbour's.", "Değerim komşumunkinden farklı."), p("That is normal: ground, building and tube differ, and one detector has a counting noise of several percent. Compare the 24-hour or 7-day value and read the pattern.",
                                                                                   "Bu normaldir: zemin, bina ve tüp farklıdır, tek bir dedektörün sayım gürültüsü yüzde birkaçtır. 24 saatlik ya da 7 günlük değere bakın, örüntüyü okuyun.")),
        (p("What is a normal value?", "Normal değer nedir?"), p("About 50 to 200 nSv/h. Granite and some volcanic rocks, stone buildings and high altitude raise it.",
                                                               "Yaklaşık 50 ile 200 nSv/h. Granit ve bazı volkanik kayaçlar, taş binalar ve yüksek rakım değeri artırır.")),
        (p("Is this an official warning system?", "Resmî bir uyarı sistemi mi?"), p("No. It is information from volunteers' hobby instruments. For official information follow your national authority.",
                                                                                  "Hayır. Gönüllülerin hobi cihazlarından gelen bir bilgidir. Resmî bilgi için ulusal otoritenizi izleyin.")),
        (p("Can I use another Geiger counter?", "Başka bir Geiger sayacı kullanabilir miyim?"), p("Yes. Besides the FNIRSI GC-03 the monitor reads the USB counters of the old Radioactive@Home project (no flashing needed) and the GQ GMC family. Only raw pulse counts are accepted. Tell us what you have: a new counter is one small driver away.",
                                                                                              "Evet. FNIRSI GC-03'ün yanında monitör eski Radioactive@Home projesinin USB sayaçlarını (yazılım yüklemeden) ve GQ GMC ailesini okur. Yalnızca ham darbe sayımı kabul edilir. Elinizdekini bize söyleyin: yeni bir sayaç küçük bir sürücü uzağınızda.")),
        (p("Can I go back to the original firmware?", "Orijinal yazılıma dönebilir miyim?"), p("Yes. The original bootloader and the factory tube calibration are never touched; copy the original file the same way.",
                                                                                              "Evet. Orijinal önyükleyiciye ve fabrika tüp kalibrasyonuna hiç dokunulmaz; orijinal dosyayı aynı yöntemle kopyalayın.")),
    ]
    faq_html = "".join(f"<details><summary>{q}</summary><p>{a}</p></details>" for q, a in faq)

    body = f"""
<header class="site"><div class="wrap bar"><a class="brand px" href="/">{flower_svg(24)}Yarrow</a>
<nav class="links"><a href="#what">{p('What', 'Nedir')}</a><a href="#how">{p('How it works', 'Nasıl çalışır')}</a><a href="#detector">{p('Detector', 'Dedektör')}</a>
<a href="#monitor">{p('Monitor', 'Monitör')}</a><a href="#hub">Hub</a><a href="#trust">{p('Trust', 'Güven')}</a><a href="#join">{p('Join', 'Katıl')}</a></nav>
<div class="lang">{langsw}</div></div></header>

<div class="hero"><div class="wrap">
<div class="pulses" aria-hidden="true"><i></i><i></i><i></i><i></i><i></i><i></i><i></i></div>
<h1 class="px">{p('A live map of <b>natural background radiation</b>', 'Doğal <b>fon radyasyonunun</b> canlı haritası')}</h1>
<p class="tag">{p("Plug a Geiger counter into your computer and join a worldwide network of volunteers. Every ten minutes real counts go to a live, open map of the background dose rate, city by city.",
                  "Bilgisayarınıza bir Geiger sayacı takın ve dünya çapında bir gönüllü ağına katılın. Her on dakikada gerçek sayımlar, şehir şehir fon doz hızının canlı, açık haritasına gider.")}</p>
<div class="cta"><a class="btn" href="https://bitboinc.athena.org.tr/yarrow.php">{p('Open the live map', 'Canlı haritayı aç')}</a>
<a class="btn alt" href="#join">{p('Join in 5 steps', '5 adımda katıl')}</a></div>
<div class="screens"><img src="/assets/hero.png" width="1240" height="320" alt="{p('Screens of the Yarrow firmware: dose rate, menu, danger alarm, field finder, sample test', 'Yarrow yazılımının ekranları: doz hızı, menü, tehlike alarmı, alan bulucu, numune testi')}"/></div>
<div class="pills px"><span class="pill">{p('Open firmware, GPL-3.0', 'Açık yazılım, GPL-3.0')}</span><span class="pill">{p('16 languages', '16 dil')}</span>
<span class="pill">Windows · Linux</span><span class="pill">{p('Raw counts only', 'Yalnızca ham sayım')}</span><span class="pill">{p('Credit for every valid bin', 'Her geçerli dilim için kredi')}</span></div>
</div></div>

<section id="what"><div class="wrap"><h2 class="px">{p('What is Yarrow?', 'Yarrow nedir?')}</h2>
<p class="sub">{p("Everywhere on Earth a little ionizing radiation passes through you all the time: gamma rays from the rocks and soil, the building you sit in, radon in the air and cosmic rays from space. It is harmless at these levels and it differs from place to place: granite reads higher than sediment, a mountain village higher than the coast. A network of cheap detectors, each watching its own corner, shows what normal looks like everywhere, so that unusual can be seen.",
                  "Yeryüzünde her yerde, her an biraz iyonlaştırıcı radyasyon sizden geçer: kayaç ve topraktan gelen gama ışınları, oturduğunuz bina, havadaki radon ve uzaydan gelen kozmik ışınlar. Bu seviyelerde zararsızdır ve yerden yere değişir: granit tortuldan, bir dağ köyü kıyıdan yüksek okur. Her biri kendi köşesini izleyen ucuz dedektörlerden oluşan bir ağ, her yerde normalin neye benzediğini gösterir; böylece olağandışı fark edilebilir.")}</p>
<div class="grid">
<div class="card"><span class="ic">📡</span><h3>{p('You measure', 'Siz ölçersiniz')}</h3><p>{p('A Geiger-Müller tube gives one pulse for each ionizing event. At background level that is about one every two seconds.', 'Bir Geiger-Müller tüpü her iyonlaştırıcı olay için bir darbe verir. Fon seviyesinde bu yaklaşık iki saniyede bir olur.')}</p></div>
<div class="card"><span class="ic">🗺️</span><h3>{p('We map', 'Biz haritalarız')}</h3><p>{p('Counts from all detectors become the median dose rate of each city, over the last hour, 3 hours, 24 hours or 7 days.', 'Tüm dedektörlerin sayımları, her şehrin son bir saat, 3 saat, 24 saat ya da 7 gündeki medyan doz hızına dönüşür.')}</p></div>
<div class="card"><span class="ic">🔎</span><h3>{p('Everyone can see', 'Herkes görür')}</h3><p>{p('The map is open to everyone, with a history graph per place and a notice when readings are clearly higher than usual.', 'Harita herkese açıktır; yer başına geçmiş grafiği ve değerler belirgin biçimde olağandan yüksekse bir uyarı sunar.')}</p></div>
</div></div></section>

<section id="how" class="alt"><div class="wrap"><h2 class="px">{p('How it works', 'Nasıl çalışır')}</h2>
<p class="sub">{p('From the tube to the map in four steps.', 'Tüpten haritaya dört adımda.')}</p>
<div class="steps">
<div class="step"><h3>{p('The detector counts', 'Dedektör sayar')}</h3><p>{p('The FNIRSI GC-03 with Yarrow firmware counts raw pulses of its J321 tube and keeps a pulse counter and a clock.', 'Yarrow yazılımlı FNIRSI GC-03, J321 tüpünün ham darbelerini sayar; bir darbe sayacı ve bir saat tutar.')}</p></div>
<div class="step"><h3>{p('The monitor cuts bins', 'Monitör dilimler')}</h3><p>{p('A small BOINC task reads the counter and cuts it into exact 10, 30 or 60 minute bins on the clock, sent every ten minutes.', 'Küçük bir BOINC görevi sayacı okur ve saate oturan tam 10, 30 ya da 60 dakikalık dilimlere böler; on dakikada bir gönderir.')}</p></div>
<div class="step"><h3>{p('The server checks', 'Sunucu denetler')}</h3><p>{p('Impossible values are refused at once. A day later each stream must pass a statistical test. Only then bins count and earn credit.', 'İmkânsız değerler hemen reddedilir. Bir gün sonra her akış istatistik testini geçmelidir. Dilimler ancak o zaman sayılır ve kredi kazandırır.')}</p></div>
<div class="step"><h3>{p('The map shows it', 'Harita gösterir')}</h3><p>{p('Cities appear as points coloured by their median dose rate. Nothing finer than a city is ever sent or shown.', 'Şehirler medyan doz hızına göre renklenen noktalar olarak görünür. Bir şehirden daha ayrıntılı hiçbir şey gönderilmez ya da gösterilmez.')}</p></div>
</div>
<div class="legend px">{legend}<span>nSv/h</span></div></div></section>

<section id="detector"><div class="wrap"><h2 class="px">{p('The detector: FNIRSI GC-03 + Yarrow firmware', 'Dedektör: FNIRSI GC-03 + Yarrow yazılımı')}</h2>
<p class="sub">{p("The FNIRSI GC-03 is a pocket Geiger counter with a J321 tube, a colour display, a rotary knob, a battery and a USB port. Yarrow is open firmware written from scratch for it, because the original gives nothing a computer could use.",
                  "FNIRSI GC-03; J321 tüplü, renkli ekranlı, döner düğmeli, bataryalı ve USB girişli cep Geiger sayacıdır. Yarrow, onun için sıfırdan yazılmış açık yazılımdır; çünkü orijinali bir bilgisayarın kullanabileceği hiçbir şey vermez.")}</p>
<div class="grid">{feat_html}</div>
<div class="screens" style="margin-top:26px"><img src="/assets/screens.png" width="920" height="1008" loading="lazy" alt="{p('All screens of the Yarrow firmware', 'Yarrow yazılımının tüm ekranları')}"/></div></div></section>

<section id="firmware" class="alt"><div class="wrap"><h2 class="px">{p('Get the firmware', 'Yazılımı edinin')}</h2>
<p class="sub">{p('One file per language. Pick yours:', 'Her dil için bir dosya. Kendinizinkini seçin:')}</p>
<div class="dl px">{dl}</div>
<ol class="num"><li>{p('Switch the detector off, hold <b>OK</b>, press the power key and plug in the USB cable.', 'Dedektörü kapatın, <b>OK</b> tuşunu basılı tutun, güç tuşuna basın ve USB kabloyu takın.')}</li>
<li>{p('A drive appears. Copy the <code>.bin</code> file of your language onto it.', 'Bir sürücü belirir. Dilinize ait <code>.bin</code> dosyasını üzerine kopyalayın.')}</li>
<li>{p('The detector restarts with Yarrow. The original bootloader and the factory tube calibration are never touched.', 'Dedektör Yarrow ile yeniden başlar. Orijinal önyükleyiciye ve fabrika tüp kalibrasyonuna hiç dokunulmaz.')}</li></ol>
<p class="sub"><a href="https://github.com/alplix/yarrow/blob/main/docs/install.md">{p('Step-by-step instructions and the source code', 'Adım adım yönergeler ve kaynak kod')}</a> · GPL-3.0</p></div></section>

<section id="monitor"><div class="wrap"><h2 class="px">{p('The monitor and the counters it reads', 'Monitör ve okuduğu sayaçlar')}</h2>
<p class="sub">{p("The BOINC part of Yarrow is a small program that uses almost no CPU. It looks for a counter every 15 seconds, cuts the pulse counter into exact bins and sends them as BOINC trickles. A bin is only kept when it is exact: both edges read in the right second, the tube voltage on the whole time, the counter not restarted, the clock not jumped. Nothing is ever estimated or padded. Builds for Windows (x64, x86, ARM64) and Linux (x86-64, ARM64, ARMv7).",
                  "Yarrow'un BOINC bölümü neredeyse hiç işlemci kullanmayan küçük bir programdır. Her 15 saniyede bir sayaç arar, darbe sayacını tam dilimlere böler ve BOINC trickle'ları olarak gönderir. Bir dilim yalnızca tam olduğunda tutulur: iki kenar da doğru saniyede okunmuş, tüp gerilimi tüm süre açık, sayaç yeniden başlamamış, saat sıçramamış. Hiçbir şey tahmin edilmez ya da doldurulmaz. Windows (x64, x86, ARM64) ve Linux (x86-64, ARM64, ARMv7) için derlemeler var.")}</p>
<table><tr><th>{p('Counter', 'Sayaç')}</th><th>{p('Connection', 'Bağlantı')}</th><th>{p('State', 'Durum')}</th></tr>
<tr><td>FNIRSI GC-03 + Yarrow</td><td>USB serial</td><td class="tag-ok">{p('Available', 'Hazır')}</td></tr>
<tr><td>{p('Radioactive@Home USB counter (SBM-20; boards 2.01 to 2.60, the kit, and 3.01)', 'Radioactive@Home USB sayacı (SBM-20; 2.01 ile 2.60 arası kartlar, kit ve 3.01)')}</td><td>USB HID</td><td class="tag-ok">{p('Available', 'Hazır')}</td></tr>
<tr><td>GQ GMC-300 / 320 / 500 / 600</td><td>USB serial</td><td class="tag-ok">{p('Available', 'Hazır')}</td></tr></table>
<p class="sub" style="margin-top:12px">{p('The rule for every counter stays the same: only raw pulse counts from the tube are accepted, never a rate the counter has smoothed itself.', 'Her sayaç için kural aynıdır: yalnızca tüpten gelen ham darbe sayımı kabul edilir, sayacın kendi yumuşattığı bir hız asla.')}</p></div></section>

<section id="hub"><div class="wrap"><h2 class="px">{p('Yarrow Hub: the desktop app', 'Yarrow Hub: masaüstü uygulaması')}</h2>
<p class="sub">{p("Yarrow Hub works on its own, without BOINC. It shows the live dose rate of your counter, keeps your history on your computer, draws graphs and statistics, shows the dose in nSv/h, µSv/h or µrem/h, rings an alarm at levels you choose and exports CSV. It runs with the FNIRSI GC-03 and with the Radioactive@Home counters. With a GC-03 it also sets the clock, calibrates the tube and installs firmware.",
                  "Yarrow Hub tek başına, BOINC olmadan çalışır. Sayacınızın canlı doz hızını gösterir, geçmişi bilgisayarınızda tutar, grafik ve istatistik çizer, dozu nSv/h, µSv/h veya µrem/h olarak gösterir, seçtiğiniz düzeylerde alarm çalar ve CSV dışa aktarır. FNIRSI GC-03 ve Radioactive@Home sayaçlarıyla çalışır. GC-03 ile ayrıca saati ayarlar, tüpü kalibre eder ve yazılım yükler.")}</p>
<div class="cta" style="justify-content:flex-start"><a class="btn" href="https://github.com/alplix/yarrow/releases/latest/download/yarrow-hub-windows-x64-setup.exe">{p('Download Yarrow Hub', 'Yarrow Hub\'ı indir')}</a>
<a class="btn alt" href="https://bitboinc.athena.org.tr/yarrow_history.php">{p('Data by city', 'Şehir bazında veriler')}</a></div></div></section>

<section id="trust" class="alt"><div class="wrap"><h2 class="px">{p('Keeping the data honest', 'Veriyi dürüst tutmak')}</h2>
<p class="sub">{p("A network of home instruments needs to trust its numbers, and Yarrow cannot lock a volunteer's computer. So it makes the checks that fake or broken data cannot pass.",
                  "Ev cihazlarından oluşan bir ağ sayılarına güvenebilmelidir ve Yarrow bir gönüllünün bilgisayarını kilitleyemez. Bu yüzden sahte ya da bozuk verinin geçemeyeceği denetimleri yapar.")}</p>
<div class="grid">{trust_html}</div>
<p class="sub" style="margin-top:16px">{p("Every valid bin is worth 10 credits (up to 150 bins per detector per day), granted about a day after the data arrived, once the checks are passed.",
                                           "Her geçerli dilim 10 kredi değerindedir (dedektör başına günde en fazla 150 dilim); veri geldikten yaklaşık bir gün sonra, denetimler geçildiğinde verilir.")}</p></div></section>

<section id="privacy"><div class="wrap"><h2 class="px">{p('Privacy', 'Gizlilik')}</h2>
<p class="sub">{p("A point on the map is a city, the name you type in your preferences. Nothing finer than a city is ever sent or shown: no address, no GPS, no street. The volunteers table shows BOINC user names and amounts, never places. City positions come from GeoNames (CC BY 4.0), the world map from Natural Earth (public domain).",
                  "Haritadaki bir nokta, tercihlerinize yazdığınız şehirdir. Bir şehirden daha ayrıntılı hiçbir şey gönderilmez ya da gösterilmez: adres, GPS ya da sokak yok. Gönüllüler tablosu BOINC kullanıcı adlarını ve miktarları gösterir, asla yer göstermez. Şehir konumları GeoNames'ten (CC BY 4.0), dünya haritası Natural Earth'ten (kamu malı) gelir.")}</p></div></section>

<section id="linux"><div class="wrap"><h2 class="px">{p('Linux and Raspberry Pi', 'Linux ve Raspberry Pi')}</h2>
<p class="sub">{p("On Linux the BOINC client runs as its own user and may not open USB devices, so the task says <b>No detector found</b> although a test program finds the counter. One command lets BOINC open the counters Yarrow reads (it only touches those):",
                  "Linux'ta BOINC istemcisi kendi kullanıcısıyla çalışır ve USB aygıtlarını açamayabilir; bu yüzden bir test programı sayacı bulsa bile görev <b>No detector found</b> der. Tek bir komut BOINC'in Yarrow'un okuduğu sayaçları açmasını sağlar (yalnızca onlara dokunur):")}</p>
<pre style="overflow-x:auto;background:var(--card,#1b2140);padding:12px 14px;border-radius:6px">curl -fsSL https://raw.githubusercontent.com/alplix/yarrow/main/linux/yarrow-linux-setup.sh | sudo sh
sudo systemctl restart boinc-client</pre>
<p class="sub">{p('Then unplug the counter and plug it in again. Works the same on Raspberry Pi OS. More details and what to look for in the task log: <a href="https://github.com/alplix/yarrow/blob/main/docs/linux.md">docs/linux.md</a>.',
                  'Sonra sayacı çıkarıp yeniden takın. Raspberry Pi OS\'ta da aynıdır. Ayrıntılar ve görev günlüğünde nelere bakılacağı: <a href="https://github.com/alplix/yarrow/blob/main/docs/linux.md">docs/linux.md</a>.')}</p></div></section>

<section id="join" class="alt"><div class="wrap"><h2 class="px">{p('Join in 5 steps', '5 adımda katılın')}</h2>
<ol class="num">
<li>{p('Get a counter: a <b>FNIRSI GC-03</b> with the Yarrow firmware (above), or use a Radioactive@Home or GQ GMC counter you already have.', 'Bir sayaç edinin: Yarrow yazılımlı bir <b>FNIRSI GC-03</b> (yukarıda) ya da elinizdeki bir Radioactive@Home veya GQ GMC sayacı.')}</li>
<li>{p('Join <a href="https://bitboinc.athena.org.tr/signup.php">BitBOINC</a> and attach your computer in the BOINC Manager.', '<a href="https://bitboinc.athena.org.tr/signup.php">BitBOINC</a>\'e katılın ve bilgisayarınızı BOINC Yöneticisi\'nde bağlayın.')}</li>
<li>{p('In <a href="https://bitboinc.athena.org.tr/prefs.php?subset=project">your project preferences</a> tick the <b>Yarrow monitor</b>, and enter your country (two letters), city, bin length and, if you like, your detector.', '<a href="https://bitboinc.athena.org.tr/prefs.php?subset=project">Proje tercihlerinizde</a> <b>Yarrow monitörünü</b> işaretleyin ve ülkenizi (iki harf), şehrinizi, dilim uzunluğunu ve isterseniz dedektörünüzü girin.')}</li>
<li>{p('Plug the detector into the computer and switch it on (a GC-03: leave it on the main screen). The monitor finds it by itself.', 'Dedektörü bilgisayara takın ve açın (GC-03 ise ana ekranda bırakın). Monitör onu kendisi bulur.')}</li>
<li>{p('Leave it running. The first bin arrives at the next 10-minute mark; the longer it runs, the better the checks and the map like it.', 'Çalışır halde bırakın. İlk dilim bir sonraki 10 dakikalık işarette gelir; ne kadar uzun çalışırsa denetimler ve harita o kadar memnun olur.')}</li></ol>
<div class="cta" style="justify-content:flex-start;margin-top:18px"><a class="btn" href="https://bitboinc.athena.org.tr/yarrow.php">{p('Open the live map', 'Canlı haritayı aç')}</a>
<a class="btn alt" href="https://bitboinc.athena.org.tr/yarrow_status.php">{p('Progress and status', 'İlerleme ve durum')}</a></div></div></section>

<section id="faq"><div class="wrap"><h2 class="px">{p('Questions', 'Sorular')}</h2>{faq_html}</div></section>

<footer><div class="wrap">{flower_svg(18)} Yarrow · {p('Open source under GPL-3.0', 'GPL-3.0 altında açık kaynak')} ·
<a href="https://github.com/alplix/yarrow">GitHub</a> · <a href="https://bitboinc.athena.org.tr">BitBOINC</a> · <a href="https://athena.org.tr">Athena</a><br>
{p('Hobby instruments of volunteers, not an official warning system.', 'Gönüllülerin hobi cihazları; resmî bir uyarı sistemi değildir.')}</div></footer>"""

    return f"""<!DOCTYPE html>
<html lang="{lang}"><head><meta charset="utf-8"/><meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>{html.escape(title)}</title><meta name="description" content="{html.escape(desc)}"/>
<link rel="canonical" href="{self_url}"/>{hreflang}
<link rel="icon" type="image/svg+xml" href="/favicon.svg"/><meta name="theme-color" content="#0b1020"/>
<meta property="og:type" content="website"/><meta property="og:site_name" content="Yarrow"/><meta property="og:title" content="{html.escape(title)}"/>
<meta property="og:description" content="{html.escape(desc)}"/><meta property="og:url" content="{self_url}"/><meta property="og:locale" content="{locale}"/>
<meta property="og:image" content="{BASE}/assets/hero.png"/><meta name="twitter:card" content="summary_large_image"/>
<script type="application/ld+json">{{"@context":"https://schema.org","@type":"SoftwareApplication","name":"Yarrow","applicationCategory":"UtilitiesApplication","operatingSystem":"Windows, Linux","license":"https://www.gnu.org/licenses/gpl-3.0.html","url":"{BASE}/","description":"{html.escape(desc)}"}}</script>
<style>{CSS}</style></head><body>{body}</body></html>"""


def main():
    os.makedirs(os.path.join(HERE, "assets"), exist_ok=True)
    for f in ("hero.png", "screens.png"):
        shutil.copy2(os.path.join(REPO, "docs", "images", f), os.path.join(HERE, "assets", f))
    for code, name, locale, fname in LANGS:
        with open(os.path.join(HERE, fname), "w", encoding="utf-8", newline="\n") as fh:
            fh.write(render(code, name, locale, fname))
    with open(os.path.join(HERE, "favicon.svg"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(flower_svg())
    with open(os.path.join(HERE, "robots.txt"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(f"User-agent: *\nAllow: /\nSitemap: {BASE}/sitemap.xml\n")
    urls = "".join(f"<url><loc>{BASE}/{f if f != 'index.html' else ''}</loc></url>" for _c, _n, _l, f in LANGS)
    with open(os.path.join(HERE, "sitemap.xml"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(f'<?xml version="1.0" encoding="UTF-8"?><urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">{urls}</urlset>\n')
    print("built", [f for _c, _n, _l, f in LANGS])


if __name__ == "__main__":
    main()
