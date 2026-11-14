# Kilo68k — checkpoint relace F2, 2026-11-13

**Aktualizace F2-RX08, 2026-11-14:** aktuální stav určují závěrečné sekce
od „F2-RX02 — hardwarová validace a audit F2-RX05“ po dokončení RX06/RX07.
Původní stav níže
(F2 hardware NOT RUN, původní další krok, zdrojové otisky a velikosti BIN)
je zachován jako historie před RX02, nikoli jako aktuální instrukce.
F2-RX02 má čtyři diagnostické 100/100 PASS a jeden normální 100/100 PASS
s ověřeným obsahem a pořadím. Nejde o automatické uzavření celého F2.

## Pokračování po restartu

Nejprve přečíst `AGENTS.md`, `docs/KILO_PORTING_CONTEXT.md`,
`docs/KILO_F3_CHECKPOINT.md` a tento soubor. Potom read-only ověřit větev,
HEAD, staging, status a otisky níže. Zachovat všechny necommitnuté změny.
Tento checkpoint je aktuálním pokračováním starších historických dokumentů:
jejich staré HEAD, velikosti binárek a tvrzení „F2 nebylo zahájeno“ již
nepopisují současný stav. Starší dokumenty nebyly v této relaci upraveny.

**Historický stav k 2026-11-13, před HW testy a RX02:** F2 ještě nebylo
testováno na fyzickém MC68010; tehdejší další etapa byla
**F2-H1b.1 — Normal Hardware Regression, NOT RUN**. Následující pokyny
zachovávají tehdejší plán, nejsou aktuálním oprávněním. Vyčkat na výsledky
uživatele, nezačínat automaticky testy ani další vývoj. Každý další hardware
test vyžaduje výslovné potvrzení uživatele. Při první reprodukovatelné regresi
zastavit a zachovat evidenci. Během hardwarové validace nejsou povoleny
zdrojové změny ani opravy bez nového schválení.

## Repozitář a zachovaný pracovní strom

- Repozitář: `/home/pavelk/rosco/rosco_m68k` (`~/rosco/rosco_m68k`).
- Větev: `feature/kilo-editor`.
- HEAD: `b7fddf44ff014bfdace3dc9e71dd9ac7f12ad9aa`.
- Tento commit obsahuje B1 + F1 + D2 + F3-I1 a byl již schválen, commitnut
  a pushnut. F2 změny níže jsou pouze lokální, necommitnuté.
- Staging je prázdný. Před vytvořením tohoto souboru byly přesně čtyři změny:

```text
 M code/software/kilo68k/main.c
 M code/software/kilo68k/terminal.c
 M code/software/kilo68k/terminal.h
 M code/software/kilo68k/tests/test.c
```

Po tomto dokumentačním kroku přibývá pouze
`?? docs/KILO_F2_SESSION_CHECKPOINT.md`. Generované build soubory jsou
ignorované; nepřidávat je do Gitu. Žádný staging, commit ani push nebyl v této
checkpointové úloze proveden. Nic neresetovat, nečistit, nestashovat a nezahazovat.

SHA-256 zdrojů ověřené čtením na konci relace; cesty jsou relativní
k `code/software/kilo68k/`:

```text
main.c
41b23bb3ca86e6afc943ae98a4c73a51489e7002204c95be64190435c719bb0a
terminal.c
0fcaca67101a55dcbc2d7e08df015cb03ad2f51a7d4fe25ec47d6fb98283912c
terminal.h
2f30606a95d22d5d857b1bfb0fbd7289f94b09ac71e54730fb32a24ffa1c02f2
tests/test.c
3435c03b0537b38dcc373b87d9be9d252c7809e5cebf087a6895d07049e16963
```

## Hardware, stabilní základ a rozsah

- Skutečné rosco_m68k MC68010, aktuální deska 5 MB fyzické RAM. Základní
  konfigurace má 1 MB; fyzická RAM neurčuje automaticky dostupnou RAM aplikaci.
  Editor na 1MB konfiguraci není hardwarově ověřen.
- Windows COM6, 38400 baud, 8N1, flow control None, local echo Off, 80×25.
  Aktuální přiřazení COM6 a nastavení potvrdit před příštím testem.
- Preferovaný Tera Term; DELETE a Ctrl+Q jsou uživatelem nakonfigurovány na
  stejné kódy jako v PuTTY. Historické odlišnosti včetně Ctrl+Q -> 0x1C
  nejsou důkazem aktuálního chování ani potvrzené příčiny. PuTTY zůstává
  srovnávacím terminálem. Python 3.14 slouží k sériovým vstupním testům.
- Stabilní libtermios 1.0: 78/78 skutečných HW scénářů PASS, neměnit.
- M2.1: hardware-validated, commit `b67ae1d`; M2.2-A: 18/18 HW scénářů PASS,
  commit `e2e8db67c6a7da6b66b0be1a8cf51d175189409c`.
- B1: statický dokument 64 řádků × 256 ASCII znaků, Enter split, žádné joining;
  Up/Down s clampováním bez preferred column; viewport 23×80, status řádek
  24, RAM ONLY warning řádek 25. Žádný heap, souborové I/O ani FAT/MicroSD
  operace uvnitř editoru.

### Zásadní upřesnění nasazení od uživatele

**Uživatel VŽDY kopíruje `.bin` na SD kartu a spouští jej z SD karty na desce.**
Nepředpokládat přenos přes COM6, Kermit ani Windows serial uploader.
COM6/Tera Term/PuTTY/Python slouží k terminálové interakci a vstupním testům.
Na příští etapu je potřeba POUZE `kilo68k.bin`; diagnostický
`kilo68k-rxdiag.bin` je vyhrazen pro pozdější etapu. Kopírování a spuštění
provádí uživatel dosavadním ověřeným postupem; konkrétní boot příkazy zde
nejsou zdokumentovány a nesmějí se vymýšlet. Nahrání programu z SD není
implementací souborového I/O editoru.

## Dokončený průběh F2

| Etapa | Stav a výsledek |
|---|---|
| F2-A1 | Read-only analýza, doporučena kombinace bounded batching a redraw coalescing |
| F2-D1 | Podrobný design schválen jako implementační základ |
| F2-I1a | Pouze terminal.c/.h: explicitní input API, sdílený parser, kompatibilní wrapper |
| F2-I1b.1 | Dosavadní normální/diagnostické host regrese, sanitizery a MC68010 build PASS |
| F2-I1b.2 | Pouze tests/test.c: 33 cílených input API scénářů, obě regrese a sanitizery PASS |
| F2-I1c | Pouze main.c a tests/test.c: dávkování, coalescing, 25 normálních / 31 diagnostických batching scénářů PASS; celé regrese, sanitizery a normální target build PASS |
| F2-H1a | Read-only příprava HW protokolu; odhalena stará F3 diagnostická binárka, chybějící historické detaily a cesta ke skriptu |
| F2-H1b.0 | Vynucené normální a diagnostické MC68010 buildy PASS, aktuální artefakty a nezměněné zdroje ověřeny |

### Současná implementace

- `terminal_read_input(int *key)` preferuje FIFO, jinak timed raw read
  VMIN=0/VTIME=1. `terminal_poll_input(int *key)` preferuje FIFO bez přepnutí;
  jinak provede jediné čtení ve VMIN=0/VTIME=0 a obnoví timed konfiguraci.
- Výsledky ERROR, EMPTY, PROGRESS, EVENT. PROGRESS/EVENT spotřebují přesně
  jeden raw bajt; EMPTY žádný. Prázdný polling nemění parser; skutečný timed
  timeout zachovává původní ESC/CSI/SS3/DISCARD/QUIET zotavení.
  `terminal_read_key()` zůstává kompatibilní wrapper (-1/0/1).
- `main.c`: maximálně 64 spotřebovaných raw bajtů na dávku včetně prvního;
  první pokus timed, další jen nonblocking poll. Žádná další timed prodleva
  uvnitř dávky. PROGRESS se počítá i bez hotové události. FIFO bajt se do
  rozpočtu počítá až při dequeue do parseru, nikoli už při pumpování.
- Události se dispatchují ihned a v pořadí. Výsledek editor_key se ORuje do
  dirty; není vynecháno jeho volání. Nejvýše jeden plný editor redraw na
  dirty dávku, před dalším timed pokusem. No-op ani samotný parser progress
  redraw nevyžadují. Žádný incremental renderer ani nová RX service policy.
- Ctrl+Q je globální ordered exit: žádné pozdější události ani pending redraw;
  společný cleanup, obnova atributů/viditelnosti kurzoru a očekávaný warm boot.
- Diagnostický Ctrl+R je bariéra bez prefetch/flush. Reset jen pro prázdný
  jednořádkový dokument, zdravý transport, parser INPUT a prázdné FIFO.
  Dávka končí i při odmítnutí; dřívější dirty se zachová a případně vykreslí.
- Diagnostický Ctrl+T je bariéra: před výstupem zmrazí snapshot, zahrnuje
  trigger do aktivních RX čítačů, zahodí pending editor redraw. Obyčejné
  události se poté ignorují; opakované Ctrl+T ukazuje stejný snapshot.
  Nedochází k automatickému obnovení editace. Ctrl+R/T jsou parserové
  události jen v původním povoleném INPUT stavu.
- D2 delivered/accepted se účtuje po každé příslušné události; accepted
  porovnává délku správného řádku. Význam čítačů ani report se nezměnily.
- F1: FIFO 512 B, pump nejvýše 16 čtení, TX po bajtu s RX servisováním.
  F3: bounded nonblocking RX během konstrukce rámce. Obojí zachováno;
  při overflow/transport/render chybě cleanup bez pending redraw.
- Renderer a editor nebyly F2 změněny: frame 4096 B, maximum 2247 B,
  stejný 80×25 výstup. Žádné nové buffery, heap ani časovače.

### Dosavadní host validace — reportované výsledky, nyní neopakováno

Před F2-I1c prošlo 10 původních normálních a 11 diagnostických skupin,
plus 33 přímých API scénářů. Po F2-I1c znovu prošly celé regrese, těchto
33 scénářů a nových 25 normálních / 31 diagnostických batching scénářů.
ASan/UBSan obou variant PASS bez nálezů. Testy používají produkční editor.c,
terminal.c a main.c s mockovaným libtermios na Ubuntu, nikoli MC68010 emulaci.
Ověřují mimo jiné 64/64/2, parser fragmenty/timeouty, bariéry, FIFO overflow,
mode/RX/TX/render chyby, event accounting a výsledných 2000 buněk i kurzor.
Host mocky neměří skutečné CPU/UART časování.

Provedené příkazy v předchozích validačních etapách:

```sh
git diff --check
make -C code/software/kilo68k/tests test
make -C code/software/kilo68k/tests test-diagnostics
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize-diagnostics
make -C code/software/kilo68k CPU=68010
```

Nejde o nově provedené testy při zápisu tohoto checkpointu ani o fyzické HW PASS.

## Připravené F2 artefakty — NEPŘEBUILDOVÁVAT

F2-H1b.0 provedlo v tomto pořadí oba doložené buildy s exit status 0:

```sh
make -B -C code/software/kilo68k CPU=68010
make -B -C code/software/kilo68k CPU=68010 PROGRAM_BASENAME=kilo68k-rxdiag EXTRA_CFLAGS=-DKILO_RX_DIAGNOSTICS
```

Kompilátor `m68k-elf-rosco-gcc (GCC) 13.3.1 20240927`, skutečné volby
`-mcpu=68010 -march=68010 -mtune=68010`. Vždy byly překompilovány editor.c,
main.c i terminal.c. Diagnostický build měl u všech `-DKILO_RX_DIAGNOSTICS`.
Žádná varování, link chyby ani nevyřešené symboly; knihovny/firmware neměněny.

| Varianta | ELF velikost | BIN velikost | text / data / BSS |
|---|---:|---:|---:|
| Normal F2 | 60840 B | 8132 B | 8122 / 4 / 37092 B |
| Diagnostic F2 | 69928 B | 10644 B | 10637 / 4 / 37280 B |

Přesné existující cesty a aktuální SHA-256, znovu ověřené pouze čtením
při tomto checkpointu:

```text
code/software/kilo68k/kilo68k.bin  (8132 B)
2887b1b76f0323ea39e496f21f466de370203cac6c4afc2fb793dc168601c98b
code/software/kilo68k/kilo68k-rxdiag.bin  (10644 B)
2069e2d17dbdb0003e976a89bf5379a8a374595e0d1807926c9912aa3bd44616
```

ELF cesty: `code/software/kilo68k/kilo68k.elf` a
`code/software/kilo68k/kilo68k-rxdiag.elf`.
Oba obsahují terminal_read_input/terminal_poll_input; diagnostický ELF navíc
terminal_diag_reset/snapshot/report. Aktuální diagnostický hash se liší od
historického F3 `a9087f63f634c2629ae8e0ef59056da0bb994b2789a9437d12f2bf187d3fdaaf`
(10416 B); tento starý otisk již neoznačuje současný soubor.

Objektové cesty jsou sdílené: po posledním diagnostickém buildu odpovídají
`.o` diagnostické variantě. To neinvaliduje již hotový normální ELF/BIN.
Nespouštět obyčejný incremental build v domnění, že automaticky obnoví normal.
Obě binárky jsou připraveny, nyní je nepřebuildovávat a nic nečistit.

## Historická hardwarová baseline F3 — 12/12 PASS

Výhradně podle uživatelem potvrzených výsledků zaznamenaných
v `docs/KILO_F3_CHECKPOINT.md`, nikoli podle nového běhu:

| Původní ID | Doložený scénář / výsledek | Stav F3 |
|---|---|---|
| F3-H1a | Python 2 ms, 100/100 přijato a vloženo | PASS |
| F3-H1b | Python 1 ms, 100/100 | PASS |
| F3-H1b-R1 | Python 1 ms opakování, 100/100 | PASS |
| F3-H2a | Tera Term rychlý Paste, 100/100 | PASS |
| F3-H2b | PuTTY rychlý Paste, 100/100 | PASS |
| F3-H3a | Down autorepeat 10 s | PASS |
| F3-H3b | Up autorepeat 10 s | PASS |
| F3-H4a | Backspace a DELETE, výsledný text AC | PASS |
| F3-H4b | Editace uprostřed řádku, výsledný text ABCDXE | PASS |
| F3-H4c | Víceřádková editace | PASS |
| F3-H4d | Scrolling přes 23řádkovou hranici | PASS |
| F3-H5a | Vizuální integrita 100 znaků a očekávané diagnostiky | PASS |

Historický Python payload je `0123456789` ×10, přesně 100 ASCII bajtů bez
CR/LF; H1a má detailnější postup v původním checkpointu. Cesta a parametry
dosavadního Windows skriptu nejsou ověřeny, odpovídající script v repozitáři
nebyl nalezen. Vyžádat od uživatele, nevymýšlet cestu/příkaz ani náhradu.
Python a terminál nesmějí současně vlastnit COM6.

Chybějící historické parametry: kompletní input trace H4a/b/c/d; počáteční
dokument/poloha a rychlost autorepeatu H3; samostatný přesný clipboard payload
H2; úplné číselné reporty H5a a jednotlivých F3 běhů. Tyto detaily označit
NOT DOCUMENTED; neprezentovat nově navržené kroky jako původní definice F3.

Před F3: PuTTY paste 59/100; Python 5 ms a 2 ms 100/100; Python 1 ms dvakrát
96/100. Po F3 nebyla v provedených scénářích pozorována ztráta. UART overrun
nebyl přímo změřen. F3 výsledek nezaručuje přenos bez ztrát při libovolné zátěži.

## Historický další krok k 2026-11-13 — již není aktuální plán

**Tehdejší stav: F2-H1b.1 — Normal Hardware Regression: NOT RUN.**
Následující postup je historický; novější výsledky a hranice oprávnění jsou
v závěrečných sekcích RX02–RX08. Nespouštět jej automaticky znovu.

1. Uživatel použije pouze výše identifikovaný normální `kilo68k.bin` 8132 B;
   před nasazením ověřit hash kopie proti tomuto checkpointu.
2. Zkopíruje jej na SD kartu, případně nahradí starou verzi.
3. Vloží SD kartu do rosco_m68k.
4. Spustí Kilo68k svým existujícím ověřeným SD-card postupem.
5. Provede a ohlásí pouze **F2-N01 — Startup and Initial Screen**.

Navržená kontrola F2-N01: prázdný dokument, 23 textových řádků bez starého
preview titulku, status na řádku 24
`Row 001/001 Len 000/256 Col 000 Off 000,000 | READY`, trvalé RAM ONLY varování
na řádku 25 a viditelný kurzor na (1,1). Idle nesmí samovolně editovat ani
periodicky překreslovat. Zaznamenat skutečnou obrazovku, konfiguraci a binární
identitu; neoznačit PASS bez uživatelova potvrzení.

**Poté zastavit a vyčkat na výsledek a potvrzení dalšího jednotlivého testu.**
Další plánované pořadí: základní editace, víceřádková editace, scrolling,
Python 2 ms a 1 ms payloady, Tera Term/PuTTY paste a další F2 testy.
Diagnostickou variantu nasadit až v pozdější zvlášť schválené etapě.

Připravené doplňkové oblasti F2-H1a: jednotlivý znak; přesné payloady délky
63/64/65/128/129/130; fragmenty CSI/SS3 a zotavení; kapacity; později D2
Ctrl+R/T bariéry, frozen report, FIFO integrity a Ctrl+Q cleanup. Délka 64
odeslaných bajtů sama neprokazuje jednu dávku na hardware. D2 není čítač
redraw; write volání, editor frames, reporty a cleanup nelze zaměňovat.
Přesné měření redraw vyžaduje vhodný raw ANSI log nebo existující host observer,
nikoli pouhý dojem rychlosti. Nic takového nyní nespouštět ani nedoplňovat.

## Rizika, evidence a hranice oprávnění

- Historicky před HW validací: F2 runtime byl NOT VERIFIED a všechny F2 HW
  testy NOT RUN. Aktuální rozsah pěti RX02 HW PASS je zaznamenán níže;
  ostatní omezení tím nejsou odstraněna.
- Méně redraw může omezit příležitosti F1/F3 RX pumpování; host testy
  negarantují skutečný UART servisní interval ani lossless přenos.
- UART overrun není přímo měřen; chybějící RX nelze automaticky takto označit.
- Hardware integrity srovnávat s F3: přesný obsah a pořadí, nejen délku;
  screen/cursor a D2 čítače jsou samostatné důkazy. Některé F3 detaily chybí.
- Známý 100bajtový digitový test po samostatném Ctrl+R a po dokončení vstupu
  má v pozdější diagnostické etapě očekávat RX total 101 včetně Ctrl+T,
  delivered/accepted/length 100, pending/overflow/error/saturated 0.
  Direct/pump/peak se mohou legitimně změnit; nejsou pevný acceptance limit.
- Nevyvolávat úmyslné elektrické či transportní poruchy. Při regresi nejprve
  uchovat vstup, stav, screenshot/log a hash; žádná automatická oprava.
- Evidovat pro každý test ID, datum/hardware, variantu a hash BIN, terminál
  či skript a pacing, počáteční stav, přesný vstup, očekávání, skutečnost,
  PASS/FAIL/INCONCLUSIVE/NOT RUN a odkazy na evidenci. Neúplný důkaz není PASS.
- Bez nového schválení žádný zdrojový zásah, build, test, stage/commit/push,
  reset/clean/stash, změna větve, firmware/libtermios změna ani další F2 vývoj.

Tento checkpoint byl vytvořen pouze dokumentační úpravou. Při jeho zápisu se
nespouštěly buildy, testy, sériový přenos ani hardware. Další akce čeká na
uživatele a jeho hardwarový výsledek.

## F2-RX02 — hardwarová validace a audit F2-RX05

Aktualizováno 2026-11-14 podle uživatelem výslovně potvrzených výsledků.
Při tomto zápisu byly pouze čteny zdroje, binárky a Git stav; host testy,
buildy ani hardware nebyly znovu spouštěny. Starší historie výše zůstává
zachována, včetně původních výsledků před RX02.

### Evidence před opravou a provedená změna

- Normální F2-N01–N07 PASS; N07 při přibližně 2 ms 100/100. N08 při 1 ms
  FAIL; kompletní text a finální Len/Col tohoto běhu nebyly zachyceny.
- Diagnostické F2-D08 při 1 ms: RX total 96 včetně Ctrl+T, delivered,
  accepted a délka 95; peak 77, pending/overflow/error 0.
- Diagnostické F2-D09.1 při 2 ms: RX total 100 včetně Ctrl+T, delivered,
  accepted a délka 99; peak 47, pending/overflow/error 0.
- Sender dokončil kontrolovaných 100 jednobajtových zápisů a flush bez
  výjimky podle uživatele. Rozestupy na UART vodiči tím nejsou změřeny.
- RX02 přidalo terminal_service_rx() po běžném EVENT/PROGRESS před další
  input operací téže dávky. Při dosažení 64 bajtů se před rendererem služba
  nepřidává. Dekódované Q/R/T bariéry se zpracují dříve; chyba vede rovnou
  do cleanup bez pending redraw. FIFO 512 B, pump <=16 čtení, parser,
  D2 definice a renderer zůstaly zachovány. V terminal.c/.h byly změněny
  pouze komentáře této služby; tests/test.c doplnilo řízené RX scénáře.

### Potvrzené hardwarové výsledky RX02

Skutečné rosco_m68k MC68010, COM6, 38400 baud, 8N1, no flow control.
Payload: 100 ASCII číslic, `0123456789` desetkrát.

| Test | Varianta | Pacing senderu | Potvrzený výsledek | FIFO peak |
|---|---|---|---|---:|
| F2-RX03.1 | Diagnostic | 2 ms | PASS, 100/100 | 47 |
| F2-RX03.2 | Diagnostic | 2 ms | PASS, 100/100 | 47 |
| F2-RX03.3 | Diagnostic | 1 ms | PASS, 100/100 | 73 |
| F2-RX03.4 | Diagnostic | 1 ms | PASS, 100/100 | 74 |
| F2-RX04 | Normal BIN | 1 ms | PASS, 100/100, obsah i pořadí ověřeny | neměřeno |

Všechny čtyři diagnostické běhy podle dodaného souhrnu měly:

```text
RX total = 101, včetně Ctrl+T
Delivered = 100
Accepted = 100
Row length = 100
FIFO pending = 0
FIFO overflow = 0
Transport error = 0
Saturated = 0
```

Rozdělení direct/pump a další jednotlivé číselné hodnoty nebyly v tomto
souhrnu dodány; nevymýšlet je. Diagnostika potvrzuje počty přijatých a
vložených bajtů, sama neporovnává jejich přesný obsah nebo pořadí.
U normálního RX04 je obsah a pořadí výslovně potvrzeno uživatelem;
diagnostické čítače pro tento normální běh nejsou dostupné.
Konkrétní názvy screenshotů/logů nebyly dodány.

Proti původním D08 95/100 a D09.1 99/100 prošly po RX02 uvedené čtyři
diagnostické běhy 100/100 a normální RX04 také 100/100. Výsledky podporují
účinnost RX02 v testovaných podmínkách. UART overrun nebyl přímo změřen
ani prokázán; nejde o záruku bezchybného příjmu při libovolném zatížení.

### Host ověření a aktuální artefakty RX02

Podle předchozího implementačního reportu, nikoli nových běhů při RX05:
celé normální/diagnostické regrese PASS, 33 API scénářů, 25/31 batching
scénářů, 6 staged-arrival streamů a 3 batch-service failure scénáře PASS;
navíc diagnostický Ctrl+R s trailing FIFO PASS. Oba ASan/UBSan PASS.
Při vývoji selhaly staré scheduling předpoklady v testech a nová kontrola
nesprávně umístěného row_length; byly opraveny v tests/test.c, finální běhy
PASS. Žádné nevyřešené runtime selhání nebylo v implementačním reportu.
Příkazy zůstaly existující test/test-diagnostics/sanitize/sanitize-diagnostics
a oba výše uvedené forced MC68010 buildy. Buildy bez varování a unresolved
symbols, normální i diagnostický exit 0. Sdílené objekty jsou diagnostické.

Aktuální soubory, velikosti podle RX02 buildu a SHA-256 ověřené čtením RX05:

| Varianta | ELF velikost | BIN velikost | text / data / BSS |
|---|---:|---:|---:|
| Normal RX02 | 60896 B | 8148 B | 8138 / 4 / 37092 B |
| Diagnostic RX02 | 69964 B | 10660 B | 10653 / 4 / 37280 B |

```text
code/software/kilo68k/kilo68k.bin
591706eb150d9b8500ed4439819007d6704ca78d49522c29252f2d87b883d6b4
code/software/kilo68k/kilo68k-rxdiag.bin
e9a597ec4fee699c4dfdd61ac677a7931e782ba6bbb36a3236d4178a6530b9dc
```

Tyto otisky nahrazují pro současné lokální artefakty starší F2-H1b.0 hodnoty
výše. Nasazení uživatel nadále provádí z SD, nikoli serial uploaderem.

### Git stav při RX05 a aktuální zdrojové otisky po RX06

Větev feature/kilo-editor, HEAD b7fddf44ff014bfdace3dc9e71dd9ac7f12ad9aa,
staging prázdný. Všechny F2/RX02 zdrojové změny jsou stále necommitnuté.

```text
 M code/software/kilo68k/main.c
 M code/software/kilo68k/terminal.c
 M code/software/kilo68k/terminal.h
 M code/software/kilo68k/tests/test.c
?? code/software/kilo68k/f2_d08_sender.py
?? code/software/kilo68k/f2_d09_2ms_sender.py
?? docs/KILO_F2_SESSION_CHECKPOINT.md
```

Sendery jsou dočasné, untracked/unstaged; NESMĚJÍ být zahrnuty do budoucího
commitu. Skutečný sender byl mezitím dodán na disk a read-only auditován;
historické tvrzení výše o jeho nedostupnosti již není aktuální.
Při přípravě D09 se změnila pouze DELAY 0.001 na 0.002.

Aktuální SHA-256, cesty relativní k code/software/kilo68k/; test.c aktualizováno
po RX06 a znovu ověřeno read-only při RX08, ostatní zdroje beze změn:

```text
main.c
24bf356503ef4974a8e1d0f1b3e55337599b7a613f996dde59c85949f5d35018
terminal.c
0011f29af6159ebd8d0760c73aca790e1b365cddd2910bda2f44f35f40457378
terminal.h
f831fdabbab03dec7b876aabc63ed8b1832f517543837f3767327539e07383e9
tests/test.c
d10e33c5d0a1415abff37b212389f05635b68a5c7c3e2a78d1fcdcdfba5fe4f5
```

### Audit RX05 a zbývající práce

- CRITICAL/MAJOR: při read-only review nebyla zjištěna potvrzená blokující
  implementační vada RX02. Nejde o formální důkaz správnosti.
- Historický MINOR, test coverage — **VYŘEŠENO v RX06**: B_RENDER_ERROR
  při RX05 nastavoval fail_set=sets+3
  (tests/test.c:1440). Po přidání batch služby první dvě mode operace
  proběhnou v ní; třetí může selhat v následujícím input pollu místo rendereru.
  Název tohoto batching scénáře proto sám nedokládá render-failure pokrytí.
  Samostatné renderer/F3 failure testy existují. Při RX05 se nic neopravovalo;
  následnou opravu a její validaci uvádí sekce RX06 níže.
- Omezení, nikoli potvrzená regrese: service je početně bounded, ne časově;
  nepokrývá vnitřek dlouhého editor_key. Při trvalém přetížení může FIFO
  explicitně přetéct. Žádná obecná lossless ani latency záruka.
- Přijatelná změna časování D2: prefetch před dosud nerozpoznaným Ctrl+R
  může reset odmítnout kvůli trailing FIFO; podmínky a pořadí zůstávají.
  RX snapshot může zahrnovat bajty za Ctrl+T, delivered/accepted však končí
  na bariéře. Testy tuto situaci kontrolují.
- Pokrytí podporuje dosavadní bounded/bariérové chování, ale host mock
  nedokládá reálné MC68010 servisní intervaly. Nových pět HW PASS automaticky
  neznamená zopakování celé původní 12scénářové F3 regrese po RX02.
- Další práci a případné rozšíření HW regresí nejprve schválit s uživatelem;
  nic nespouštět automaticky. Celé F2 není tímto checkpointem uzavřeno.
- **Žádný commit ani push nebyl autorizován.** Staging/commit/push, zdrojové
  opravy, další testy či buildy vyžadují nové výslovné instrukce.

### F2-RX06 — dokončená cílená oprava testu

Upraven pouze code/software/kilo68k/tests/test.c. B_RENDER_ERROR již
nepoužívá fail_set=sets+3. Mock rosco_termios_write rozpozná prefix
následného editorového rámce a deterministicky vrátí chybu během renderer TX,
po úspěšně odeslaném resetu atributů. Explicitní příznak a assertions
vyžadují skutečný zásah injekce, chybový návrat kmain, jediný dokončený
úvodní rámec a cleanup s viditelným kurzorem, restore a unbind.
Pouhé dřívější selhání input pollu test nesplní. Jde o renderer TX failure,
nikoli o mode-switch failure uvnitř konstrukce rámce.

Podle dokončené validace RX06 všechny následující příkazy PASS:

```sh
git diff --check
make -C code/software/kilo68k/tests test
make -C code/software/kilo68k/tests test-diagnostics
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize-diagnostics
```

Obě host konfigurace výslovně potvrdily dosažení renderer TX a cleanup;
prošly také ostatní regrese včetně 25/31 batching scénářů. Oba ASan/UBSan
bez nálezů. Produkční kód, checkpoint, sendery a BIN soubory při RX06
zůstaly beze změny; MC68010 build ani hardware test se neprováděl.
Tyto výsledky nejsou nové běhy při synchronizaci RX08.

### F2-RX07 — dokončený read-only pre-commit audit

- Žádná potvrzená blokující produkční vada. Zachovány rozpočty 64 raw bajtů,
  pump <=16, FIFO 512 B, Q/R/T bariéry, FIFO pořadí a cleanup při chybě.
- Pět skutečných MC68010 validačních scénářů RX03.1–RX03.4 a RX04 PASS
  v rozsahu tabulky výše; nejde o univerzální spolehlivost ani důkaz overrun.
- Oprava B_RENDER_ERROR je ve zdroji přítomna; zbývala dokumentační
  synchronizace RX06, stavu MINOR a otisku test.c, provedená nyní v RX08.
- Python sendery f2_d08_sender.py a f2_d09_2ms_sender.py zůstávají lokální,
  untracked/unstaged a jsou výslovně vyloučeny z návrhu commitu.
- Generované BIN, ELF, objekty, mapy, disassembly, symboly a host build
  výstupy jsou ignorované a vyloučené z návrhu commitu. BIN hashe souhlasily.
- Navržený budoucí rozsah je přesně main.c, terminal.c, terminal.h,
  tests/test.c pod code/software/kilo68k/ a tento checkpoint.
  Návrh zprávy: feat(kilo68k): batch input with bounded RX servicing.
  Tento návrh není oprávněním k provedení Git operací.
- Žádný commit ani push nebyl autorizován; staging zůstává prázdný.

RX08 upravuje pouze tento dokument. Původní historie a důkazy zůstaly
zachovány. Další krok vyžaduje výslovné schválení uživatele; žádné automatické
staging/commit/push ani další testy či implementace.
