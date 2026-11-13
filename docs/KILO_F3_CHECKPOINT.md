# Kilo68k — persistentní checkpoint F3-I1

Zapsáno 2026-11-12; aktualizováno 2026-11-13 podle uživatelem potvrzených
výsledků skutečných HW testů. **F3-I1: hardware-validated, 12/12 PASS.**
Tento soubor je uložen v repozitáři na disku; není závislý na historii chatu.

## Instrukce pro novou relaci

1. Před jakoukoli vývojovou akcí přečíst `AGENTS.md`,
   `docs/KILO_PORTING_CONTEXT.md` a tento checkpoint.
2. Ověřit větev a status read-only. Zachovat všechny existující změny.
3. Historii M2.2-A hledat v `docs/KILO_DEVELOPMENT_CHECKPOINT.md`, aktuální
   funkce také v `code/software/kilo68k/README.md`; starší seznamy dalších
   kroků nepovažovat za nové oprávnění.
4. F3-H1a a dalších jedenáct scénářů níže jsou dokončeny, všechny PASS.
   Nyní vyčkat na další instrukce; nespouštět nové testy, F2 ani změny bez
   výslovného schválení. Uživatel provádí fyzické hardware testy.
5. Bez výslovného schválení žádné staging/commit/push/merge/reset/clean/stash,
   změna větve, zahazování práce ani zásah do stabilních komponent.

## Repozitář a historie

- Cesta: `/home/pavelk/rosco/rosco_m68k` (`~/rosco/rosco_m68k`).
- Větev: `feature/kilo-editor`.
- HEAD: `e2e8db67c6a7da6b66b0be1a8cf51d175189409c`.
- Subject: `feat(kilo68k): add RAM-only single-line editor M2.2-A`.
- M2.2-A byl commitnut a pushnut; 18/18 plánovaných HW scénářů PASS.
- M2.1: commit `b67ae1d38e48e4ff185305be3adad7315795265b`, HW ověřen.
  Diagnostika `code/software/kilo_terminal_test/` zůstává nedotčená.
- Origin fork: `https://github.com/PavelKCZ/rosco_m68k.git` (historický kontext;
  remote ani síť se při tomto dokumentačním checkpointu neověřovaly).
- Upstream Kilo: `~/rosco/kilo-upstream`; původní zdroj neměnit.
- libtermios 1.0: `code/software/libs/src/termios/`, **78/78 skutečných HW
  testů PASS**. Stabilní základ, neměnit.

## Hardware a rozsah aplikace

- rosco_m68k, MC68010, aktuální testovací deska s 5 MB fyzické RAM.
- Potvrzené konfigurace: základ 1 MB, rozšíření 5 MB. Fyzickou kapacitu
  neztotožňovat s RAM dostupnou aplikaci; linker/startup/stack nejsou tím ověřeny.
- Windows, COM6, 38400 baud, 8N1, flow control None, 80×25, local echo Off.
  Tera Term je nyní preferovaný; DELETE a Ctrl+Q jsou uživatelem nastaveny
  na stejné kódy jako v PuTTY. PuTTY byl rovněž použit v úspěšných F3 testech.
- Statický dokument: 64 řádků, nejvýše 256 ASCII znaků na řádek plus NUL.
  Enter rozděluje řádek; CR je Enter, LF ignorován. PuTTY CR ověřeno M2.1.
- Up/Down s clampováním cx, bez preferred column; horizontální i vertikální
  viewport. 23 textových řádků, status 24, RAM ONLY warning 25.
- Backspace/Delete/Left/Right nepřecházejí přes hranice řádků; joining odložen.
- Žádný heap, souborové I/O, FAT/MicroSD, syntax highlighting ani search.
- H01–H11 (B1.3b) **11/11 PASS před F3**, potvrzeno uživatelem.
  Toto tvrzení není F3 hardwarovou regresí.

## F1, D2 a F3-I1

### F1 — již HW ověřený transport

`terminal.c` vlastní konfiguraci, parser, FIFO a transportní chybu.
FIFO 512 B; pumpa nejvýše 16 čtení. TX po jednom bajtu, RX polling před TX
smyčkou a po každém bajtu. Přepnutí na raw VMIN=0/VTIME=0 během TX,
obnova VMIN=0/VTIME=1. Overflow/chyba se latchuje a vede do cleanupu.
FIFO se spotřebuje před přímým RX. H07 vložené A/B při autorepeatu byl F1
opraven; krátký i dlouhý autorepeat uživatel ověřil na hardware.

### D2 — dočasná instrumentace

Jen pod `KILO_RX_DIAGNOSTICS`. Saturující uint32 čítače direct RX, pump RX,
enqueue, overflow, printable delivered/accepted; peak/pending FIFO,
parser/transport stav, saturace a informace o zahrnutí Ctrl+T.
Accepted porovnává délku správného řádku před a po vložení.

- Ctrl+R: tichý reset pouze při zdravém transportu, parser INPUT, prázdné FIFO
  a prázdném jednořádkovém dokumentu. Žádné TX, polling ani flush navíc.
- Ctrl+T: snapshot před jakýmkoli reportem; zmrazen do dalšího resetu.
  Přijatý Ctrl+T je při aktivním měření zahrnut do RX a report to uvádí.
  Při zobrazení reportu se další text nevkládá.
- Ctrl+Q: původní exit/cleanup; očekávaný warm boot není chyba.
- Příkazy interpretuje main.c, nikoli RX pumpa. Během paste žádný nový výstup.

### F3-I1 — implementováno a hardware-validated (12/12 PASS)

Změny této etapy pouze:
`code/software/kilo68k/terminal.c`, `terminal.h`, `editor.c`, `tests/test.c`.
Main.c a D2 definice zůstaly během F3 beze změn.

`terminal_service_rx()` je veřejný bounded wrapper: ověří transport, přes
TCSANOW přepne na polling, jednou zavolá F1 pumpu a vždy zkusí obnovit timed
raw režim. Jakákoli chyba se latchuje a vrací -1. Žádný TX, parser ani dispatch.
Volá se synchronně z rendereru, nikoli vnořeně do F1 výstupu nebo cleanupu.

Servisní body: po základní validaci, před každým z 23 fyzických textových
řádků včetně prázdných, po blocích <=16 viditelných znaků, před/po statusem,
před/po warningu a před finálním TX. Nejvýše 80 viditelných znaků na řádek.
Chyba zastaví konstrukci; rozpracovaný rámec se neodešle. Main.c již propaguje
editor_render() == -1 do cleanupu.

Rámec je bajtově shodný s pre-F3 plným rámcem: 2247 B maximum, statický buffer
4096 B. Dokument se během renderu nemění. F2 nebylo zahájeno: stále jeden
redraw na každou změnu.

| Rámec | RX služby při konstrukci | Termios přepnutí včetně F1 TX |
|---|---:|---:|
| Prázdný | 28 | 58 |
| Jeden řádek, 80 viditelných znaků | 33 | 68 |
| 23×80 plný viewport | 143 | 288 |

Počty nezahrnují open/cleanup. Režie přepínání není na MC68010 změřená.

## B1-P01 — historická evidence PŘED F3

| Test | Způsob | Výsledek |
|---|---|---|
| D3 | Rychlý PuTTY paste | 59/100 FAIL |
| D5b | Python 5 ms mezi bajty | 100/100 PASS |
| D6a | Python 2 ms mezi bajty | 100/100 PASS |
| D6b | Python 1 ms mezi bajty | 96/100 FAIL |
| D6b-R1 | Python 1 ms opakovaně | 96/100 FAIL |

Oba 1 ms běhy daly shodný report:
RX direct 2, pump 95, total 97 včetně Ctrl+T; enqueue 95, peak 94, pending 0,
overflow 0, delivered 96, accepted 96, length 96, parser 0, transport error 0.

Rychlý paste: direct 2, pump 58, total 60 včetně Ctrl+T; enqueue/peak 58,
pending/overflow 0, delivered/accepted/length 59, transport error 0.
První část textu obsahovala `012356789...`; chybějící znaky nedorazily ani po
čekání. Ruční psaní 100 znaků fungovalo (Len/Col 100, Off 000,021 po End).

Potvrzeno: všechny přijaté payload bajty byly vloženy; část vstupu nebyla
úspěšně vrácena aplikaci z libtermios. Aplikační FIFO overflow nebyl pozorován.
**UART overrun je hypotéza, nikoli potvrzená diagnóza.** Ztrátu v hostiteli,
adaptéru nebo jiné vrstvě před úspěšným čtením tato instrumentace nerozliší.

Zdrojové opory: firmware `stage1/duart.asm` v
`code/firmware/rosco_m68k_firmware/`: D_CHECKCHAR_DUART_A maskuje RX-ready,
D_RECVCHAR_DUART_A ignoruje overrun, D_SENDCHAR_DUART_A čeká na TXEMT.
Libtermios raw_read() polluje firmware bez vlastní raw RX fronty.
Repo uvádí XR68C681; jeho RX FIFO je malá (3 B + samostatný RSR).
Aktivní čip/firmware a OE na testované desce nejsou nezávisle potvrzeny.
Při 38400 8N1 je bajt ~260 us; host testy negarantují servisní intervaly.

## Dosavadní host ověření a buildy F3-I1

Následující příkazy byly provedeny v předchozí implementační části relace;
finální výsledky PASS. Při psaní tohoto checkpointu se nic znovu nespouštělo.

```sh
make -C code/software/kilo68k/tests test
make -C code/software/kilo68k/tests test-diagnostics
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize
ASAN_OPTIONS=detect_leaks=0 make -B -C code/software/kilo68k/tests sanitize-diagnostics
make -B -C code/software/kilo68k CPU=68010 PROGRAM_BASENAME=kilo68k-rxdiag EXTRA_CFLAGS=-DKILO_RX_DIAGNOSTICS
make -B -C code/software/kilo68k CPU=68010
```

Native Ubuntu cc kompiluje produkční editor.c, terminal.c a main.c s mocky
libtermios. Sanitizery ASan/UBSan se vztahují i na produkční C soubory.
Testy pokrývají všechny konstrukční servisní body, FIFO hranice, limit 16,
fragmenty ESC, pořadí příkazů, nezměněný dokument/frame a selhání přepnutí
na každém servisním bodu. ANSI model kontroluje všech 2000 buněk obrazovky.
Mock doručuje podle TX/polling příležitostí; neměří reálný čas CPU/UART.

Počáteční chybný předpoklad nového overflow testu byl opraven: prázdný rámec
má jen 28 služeb a 513 bajtů by nepřeteklo před TX; pro tuto kontrolu se nyní
používá plný viewport. Finální běhy včetně vynuceně obnoveného diagnostického
sanitizeru PASS, bez nevyřešeného selhání.

Oba target buildy bez varování a nevyřešených symbolů, CPU/ARCH/TUNE=68010.
Při střídání variant je nutné vynutit rebuild (-B): objektové soubory jsou
sdílené. Poslední objekty a kilo68k.bin jsou běžná varianta; diagnostický BIN
je zachován samostatně. Nyní nepřebuildovávat jen kvůli restartu.

| Varianta | ELF velikost | BIN velikost | text / data / BSS |
|---|---:|---:|---:|
| Normal | 58524 B | 7988 B | 7978 / 4 / 37092 B |
| Diagnostics | 67096 B | 10416 B | 10393 / 4 / 37280 B |

Cesty: `code/software/kilo68k/kilo68k.elf`, `kilo68k.bin`,
`code/software/kilo68k/kilo68k-rxdiag.elf`, `kilo68k-rxdiag.bin`.
BSS proti pre-F3 beze změn. F3 hardwarové výsledky jsou zaznamenány níže.

## F3 — potvrzené hardwarové výsledky, 12/12 PASS

Zaznamenáno 2026-11-13 výhradně podle výsledků výslovně potvrzených uživatelem.
Skutečné rosco_m68k MC68010, aktuální testovací deska 5 MB RAM; COM6,
38400 baud, 8N1, bez flow control, terminál 80×25. Použity Python, Tera Term
a PuTTY podle jednotlivých scénářů.

| Test | Potvrzený scénář / pozorovaný výsledek | Výsledek |
|---|---|---|
| F3-H1a | Python 2 ms, přijato a vloženo 100/100 znaků | PASS |
| F3-H1b | Python 1 ms, 100/100 | PASS |
| F3-H1b-R1 | Python 1 ms opakování, 100/100 | PASS |
| F3-H2a | Tera Term rychlý Paste, 100/100 | PASS |
| F3-H2b | PuTTY rychlý Paste, 100/100 | PASS |
| F3-H3a | Šipka dolů, autorepeat 10 s | PASS |
| F3-H3b | Šipka nahoru, autorepeat 10 s | PASS |
| F3-H4a | Backspace a DELETE, výsledný text `AC` | PASS |
| F3-H4b | Editace uprostřed řádku, výsledný text `ABCDXE` | PASS |
| F3-H4c | Víceřádková editace | PASS |
| F3-H4d | Scrolling přes 23řádkovou hranici | PASS |
| F3-H5a | Vizuální integrita 100 znaků a očekávané diagnostiky | PASS |

Před F3: PuTTY Paste 59/100; Python 1 ms 96/100 ve dvou bězích.
Po F3 nebyla v provedených scénářích pozorována ztráta znaků.
Detailní číselné reporty každého nového scénáře nebyly v tomto souhrnu dodány;
nevydávat staré očekávané hodnoty za samostatně zachycená nová měření.

**Rozsah důkazu:** F3-I1 je hardware-validated pro výše uvedených 12 scénářů.
UART overrun nebyl přímo změřen a příčina nižšího RX před F3 tím není
jednoznačně prokázána. F3 nezaručuje bezchybný přenos při libovolném zatížení
ani formální paměťovou bezpečnost. Nejde o automatické uzavření celého B1.

Tera Term je nyní preferovaný terminál. DELETE a Ctrl+Q jsou nakonfigurovány
na stejné kódy jako v PuTTY. Historické odlišnosti jsou zachovány níže jako
historie; nelze z nich vyvozovat aktuální chování této konfigurace ani
potvrzenou původní příčinu. Aplikace ani libtermios kvůli nim nemají workaround.

Další postup není automaticky autorizován: **zastavit a vyčkat na instrukce**.
F2, další implementace, nové testy ani stage/commit/push se nyní neprovádějí.

## Historický postup a acceptance kritéria F3-H1a — dokončeno PASS

Následující postup byl připraven před testem a je zachován pro reprodukci;
není aktuálním pokynem test znovu spustit.

Použít **existující** `code/software/kilo68k/kilo68k-rxdiag.bin`, **10416 B**.

1. Spustit tento nový diagnostický BIN na MC68010; původní sériové nastavení.
2. Začít s prázdným dokumentem a nechat ustálit úvodní obrazovku.
3. Stisknout Ctrl+R (reset je tichý).
4. Zavřít PuTTY, aby uvolnilo COM6.
5. Spustit již použitý Python skript s **2 ms pacingem**, který pošle přesně
   `0123456789` desetkrát (100 ASCII bajtů, bez CR/LF).
6. Vyčkat na dokončení skriptu a uvolnění COM6.
7. Znovu otevřít PuTTY se stejnými parametry a počkat na dokončení zpracování.
8. Stisknout Ctrl+T.
9. Zaznamenat celý diagnostický report, text/status před snapshotem a výsledek.

Umístění/název již použitého Windows Python skriptu nebyly v této relaci
ověřeny; v repozitáři nebyl nalezen odpovídající skript. Získat cestu od
uživatele, pokud nebude k dispozici. Nevymýšlet ji a nevytvářet náhradu bez
schválení. Python a PuTTY nesmějí současně vlastnit COM6.

PASS: RX total **101** včetně Ctrl+T; delivered **100**, accepted **100**,
row length **100**; FIFO pending **0**, FIFO overflow **0**, transport error
**0**, saturated **0**. Ověřit přesné pořadí 100 číslic. RX direct, RX pump
(a jejich rozdělení), enqueue a peak se mohou proti pre-F3 změnit; očekává se
součet direct+pump 101. Jejich staré konkrétní hodnoty nejsou acceptance limit.

Původní bod zastavení po F3-H1a byl následně překročen uživatelem potvrzenými
výše zaznamenanými testy. Nyní se čeká na nové instrukce. Ctrl+Q zůstává
standardní cestou ukončení a obnovy terminálu.

## Omezení a zbývající rizika

- F3 nemusí odstranit všechny ztráty; service intervaly ani overrun nezměřeny.
- Přidané polling/mode-switch operace i D2 čítače mění časování.
- Bez F2 může při souvislém vstupu narůstat FIFO až k explicitní chybě.
- Tera Term 5.6.1 Ctrl+Q vedlo k 0x1C místo 0x11; příčina není potvrzena.
  V PuTTY Ctrl+Q funguje; 0x1C se neinterpretuje jako Quit.
- Tera Term Delete bylo pozorováno jako Backspace; přesné bajty nezachyceny,
  příčina není potvrzena. Tato pozorování popisují historické nastavení;
  aktuálně je preferován Tera Term s DELETE/Ctrl+Q nastavenými jako v PuTTY.
- Neúplné/malformed ESC a všechny chyby nejsou komplexně ověřeny na hardware.
- 1 MB konfigurace není HW ověřena pro tento editor. Testy nejsou formální
  důkaz paměťové bezpečnosti. F3 má výše přesně vymezených 12 HW PASS.

## Git stav při původním uložení (2026-11-12)

Před dokumentačním checkpointem byly následující změny již přítomné:

```text
 M code/software/kilo68k/README.md
 M code/software/kilo68k/editor.c
 M code/software/kilo68k/editor.h
 M code/software/kilo68k/main.c
 M code/software/kilo68k/terminal.c
 M code/software/kilo68k/terminal.h
?? code/software/kilo68k/tests/
```

Tests obsahuje `.gitignore`, `Makefile`, `test.c`, `ansi.c`, `ansi.h`.
Generované build výstupy jsou ignorované a nesmějí se přidávat do commitu.
Staging byl prázdný. Tento checkpoint navíc mění pouze `AGENTS.md`,
`docs/KILO_PORTING_CONTEXT.md` a přidává `docs/KILO_F3_CHECKPOINT.md`.
Vše je lokální a necommitnuté; žádný stage, commit ani push nebyl proveden.
Dokumentační aktualizace 2026-11-13 upravila také aplikační README; zdroje,
testy a binárky se neměnily. Pracovní větev zůstává feature/kilo-editor.
Dočasné /tmp baseline soubory nejsou nutné pro pokračování a nemusí přežít
restart. Rozhodující je tento dokument a zachovaný pracovní strom.

## Otisky existujících binárek při checkpointu

Ověřeno pouze čtením souborů, bez rebuildu:

- `code/software/kilo68k/kilo68k.bin`: 7988 B, SHA-256 `eb3ead075eb9c6b9fbb522c478f589556cb68fd12660d79fb341dc5070e340e8`.
- `code/software/kilo68k/kilo68k-rxdiag.bin`: 10416 B, SHA-256 `a9087f63f634c2629ae8e0ef59056da0bb994b2789a9437d12f2bf187d3fdaaf`.

## Otisky zdrojů pro ověření zachování po restartu

SHA-256, kořen cest `code/software/kilo68k/`. Zdrojové a testovací otisky jsou
z původního checkpointu; README otisk aktualizován po dokumentační změně
2026-11-13. Binární otisky výše zůstávají historickým ověřením bez nového buildu:

- `main.c`: `fa91762cb5a270e8ccc22f6f84c5550fddbb9fbfa60adfb9375b016fa70270a3`
- `editor.c`: `8e0f40b3db514b3474193650b775cb17c46ae0b3296a3c05a30f47e1a671687c`
- `editor.h`: `8c286f4df56ba744832f24f5260385cf8d7d870f479d8fb51a060b6a6645a707`
- `terminal.c`: `e4b5634cfb5a29c6b2393968d50a0a27da45dc01e55abc1d682cba58935231c4`
- `terminal.h`: `d7d80b4c554d3329561c89ec82d4ff566ac89d551903e95da5be41ba1eef4482`
- `Makefile`: `0b9c036da045c406b2879e6bd7ebf083021f4bf4ab68f88be47edee716f67e3b`
- `README.md`: `024a7abf68b6c6e37ff16501edc0984818bf1849068e8feb940c2bda47783c34`
- `tests/test.c`: `c2b238aae43d60b7993bfa29fe4835736e016305d9e0c2e9e88b60f27cda887d`
- `tests/Makefile`: `a81778b7cdbf196e5d6562c2ce2da7b03840746cf710ff1a80357fc00e9e1af3`
- `tests/ansi.c`: `88e7820489857b2084814e99bf0d0013ed33f54a78206a595949787b9371b57f`
- `tests/ansi.h`: `3fab01326a80dc5d98077fcb4c4027f9c1021cf0efbfffa0b7068f8507bd6c17`
- `tests/.gitignore`: `020c2606e8ed484089c2083e0be438b9d0736bee1558a6b76af0d58bd23f69cf`
