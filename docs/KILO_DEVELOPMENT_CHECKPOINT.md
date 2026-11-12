# Kilo68k — checkpoint pro pokračování po restartu

Zapsáno 2026-11-11, aktualizováno 2026-11-12 po potvrzení HW Testů 01–07-B.
Dokument zachycuje stav vývoje a níže uvedený rozsah hardwarového ověření.

## Nejdůležitější další krok

**M2.2-A HW Testy 01, 02-A, 02-B, 03, 04-A, 04-B, 04-C, 05-A, 05-B,
05-C1, 05-C2, 06-A, 06-B1, 06-B2, 06-C1, 06-C2, 07-A a 07-B: PASS — potvrzeno uživatelem
na skutečném hardware.**
Zaznamenané testy jsou dokončeny. Nyní čekat na další instrukce; bez dalšího
pokynu nepokračovat širším testováním, implementací dalších etap ani commitem.

## M2.2-A HW Test 01 — PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM. Terminál: PuTTY.

Uživatel potvrdil:

- Editor se spustí a obrazovka i stavové řádky se vykreslují správně.
- Kurzor zůstává viditelný během psaní.
- Sekvence `abcd` → Left dvakrát → `X` vytvoří přesně `abXcd`.
- Kurzor je viditelně bezprostředně za `X`, před `c`.
- Stav uvádí **Length 005/256, Cursor 003, View offset 000**.

**Hodnocení: PASS pro tento scénář.** Nejde o uzavření celé M2.2-A ani
potvrzení ostatních scénářů: zaplnění na 256 znaků, scrolling,
zotavení parseru a ukončení/obnova terminálu tímto testem ověřeny nejsou.
Mazání má samostatný záznam v HW Testu 02 níže.

### Reprodukovatelný postup dokončeného HW Testu 01

1. Připojte PuTTY: Windows, COM6, 38400 baud, 8N1, flow control **None**,
   local echo **Off**, terminál **80×25**.
2. Na skutečné testovací desce rosco_m68k, **MC68010 s 5 MB fyzické RAM**,
   nahrajte a spusťte existující soubor
   `code/software/kilo68k/kilo68k.bin` obvyklým postupem rosco_m68k.
3. Ověřte titulek editoru, stavové informace, nápovědu, trvalé varování
   RAM ONLY a viditelný kurzor v prázdném editačním řádku 5.
   Stav začíná Length 000, Cursor 000, View offset 000.
4. Napište `abcd`.
5. Stiskněte dvakrát šipku Left.
6. Napište `X`.
7. Ověřte text **`abXcd`**, kurzor bezprostředně za `X` (před `c`).
   Očekávaný stav: Length 005, Cursor 003, View offset 000;
   fyzická pozice kurzoru řádek 5, sloupec 4.
8. **Zde zastavte a nahlaste výsledek před dalším postupem.** Uveďte
   skutečný text, polohu kurzoru, případné odchylky a použitý terminál.
   Výsledek uživatel potvrdil; záznam PASS je uveden výše.

Podrobný širší plán je v `code/software/kilo68k/README.md`, ale aktuální
první test je již dokončen a další postup čeká na instrukce. Ctrl+Q je dostupný
jako okamžitý konec se ztrátou RAM textu; není součástí výše požadovaného
prvního editačního testu.

## M2.2-A HW Test 02 — mazání

Výsledky zaznamenány 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.

### HW Test 02-A — Backspace: PASS

- Výchozí text: `abcde`.
- Postup: Left dvakrát, potom Backspace.
- Potvrzený výsledek: **`abde`**.
- Potvrzený stav: **Length 004/256, Cursor 002, View offset 000**.

### HW Test 02-B — Delete v PuTTY: PASS

- Terminál: PuTTY.
- Výchozí text: `abcde`.
- Postup: Left dvakrát, potom Delete.
- Potvrzený výsledek: **`abce`**.
- Potvrzený stav: **Length 004/256, Cursor 003, View offset 000**.

### Pozorování v Tera Termu — příčina nezjištěna

Delete se chovalo jako Backspace. Přesné odeslané bajty dosud nebyly
zachyceny. Toto pozorování nepotvrzuje chybu implementace editoru ani
konkrétní příčinu na straně terminálu. Bez zachycení bajtů nelze příčinu
uzavřít; žádná úprava aplikace nebo knihoven nebyla provedena.

PASS platí pro výše popsané scénáře; nejde o úplné ověření všech poloh
mazání, kapacitních hranic ani uzavření celé M2.2-A. Čekat na další instrukce.

## M2.2-A HW Test 03 — Home a End: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM. Terminál: PuTTY.

- Home správně přesune kurzor na začátek editovatelného řádku.
- End správně přesune kurzor na konec editovatelného řádku.
- Obě klávesy fungují správně: **PASS**.

Přesné hodnoty stavového řádku nebyly pro tento test samostatně zaznamenány;
neuvádějí se jako nezávisle zachycené výsledky. Test nepotvrzuje uzavření
celé M2.2-A. Čekat na další instrukce.

## M2.2-A HW Test 04-A — horizontální scrolling doprava: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.
Terminál: PuTTY, 38400 baud, 8N1.

Postup:

1. Spustit editor s prázdným řádkem.
2. Zadat přesně 85 ASCII znaků `a`.
3. Pozorovat stavový řádek a fyzický kurzor.

Potvrzené výsledky:

- **Length 085/256, Cursor 085, View offset 006**.
- Kurzor zůstal viditelný na editovatelném řádku.
- Nedošlo k zalomení řádku.

**Hodnocení: PASS.** Horizontální scrolling doprava za hranici 80sloupcového
terminálu v tomto testu funguje správně. Výsledek nepotvrzuje scrolling
vlevo, kapacitní hranici 256 znaků ani další nezaznamenané scénáře.
Čekat na další instrukce.

## M2.2-A HW Test 04-B — návrat viewportu na začátek: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM. Terminál: PuTTY.

- Výchozí stav: **Length 085/256, Cursor 085, View offset 006**.
- Akce: jednou stisknout Home.
- Pozorovaný stav: **Length 085/256, Cursor 000, View offset 000**.
- Kurzor se vrátil na začátek editovatelného řádku.

**Hodnocení: PASS.** Horizontální viewport se při navigaci na začátek
dlouhého řádku správně vrátí na offset 000. Test potvrzuje návrat pomocí
Home, nikoli všechny varianty scrollingu vlevo nebo uzavření celé M2.2-A.
Čekat na další instrukce.

## M2.2-A HW Test 04-C — návrat na konec dlouhého řádku: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM. Terminál: PuTTY.

- Výchozí stav: **Length 085/256, Cursor 000, View offset 000**.
- Akce: jednou stisknout End.
- Pozorovaný stav: **Length 085/256, Cursor 085, View offset 006**.
- Kurzor se vrátil na konec editovatelného řádku.

**Hodnocení: PASS. HW Testy 04-A, 04-B a 04-C všechny PASS.** Horizontální
scrolling a navigace Home/End přes hranici 80sloupcového viewportu jsou
ověřeny pro testovaný řádek o 85 znacích. Tento výsledek nepotvrzuje jiné
délky, všechny způsoby navigace ani uzavření celé M2.2-A.
Čekat na další instrukce.

## M2.2-A HW Test 05-A — dosažení kapacity 256 znaků: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.
Terminál: PuTTY, 38400 baud, 8N1.

Postup:

1. Spustit kilo68k s prázdným bufferem.
2. Zadat přesně 256 ASCII znaků `a`.

Pozorované výsledky:

- **Length 256/256, Cursor 256, View offset 177**.
- Kurzor zůstává viditelný na konci editovatelného řádku.
- Nedošlo k zalomení řádku.

**Hodnocení: PASS.** Editor úspěšně dosáhl maximální kapacity 256 znaků
bez pozorované chyby zobrazení nebo kurzoru. Odmítnutí 257. znaku tento
test neověřuje; má samostatný záznam v HW Testu 05-B níže.
Nejde o uzavření celé M2.2-A.
Čekat na další instrukce.

## M2.2-A HW Test 05-B — odmítnutí vložení nad kapacitu: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.
Terminál: PuTTY, 38400 baud, 8N1.

Výchozí stav:

- Buffer obsahuje přesně 256 ASCII znaků `a`.
- **Length 256/256, Cursor 256, View offset 177**.

Akce: pokusit se vložit jeden další ASCII znak `X`.

Pozorované výsledky:

- **Length 256/256, Cursor 256, View offset 177** — beze změny.
- Editor zůstal funkční.
- Diagnostická hláška: `FULL - insertion rejected; text and cursor unchanged`.

**Hodnocení: PASS.** Editor správně odmítl vložení nad kapacitu 256 znaků
a zachoval pozorovaný stav editoru. **HW Testy 05-A a 05-B oba PASS.**
Tento scénář ověřuje odmítnutí při kurzoru na konci plného řádku;
nepotvrzuje odmítnutí ve všech ostatních pozicích ani uzavření celé M2.2-A.
Čekat na další instrukce.

## M2.2-A HW Test 05-C — uvolnění místa a opětovné vložení: PASS

Konečné výsledky zaznamenány 2026-11-12 na základě výslovného potvrzení
uživatele. Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.
Terminál: PuTTY, 38400 baud, 8N1.

### HW Test 05-C1 — Backspace z plného řádku: PASS

- Výchozí stav: **Length 256/256, Cursor 256, View offset 177**.
- Akce: jednou stisknout Backspace.
- Pozorovaný stav: **Length 255/256, Cursor 255, View offset 177**.
- Kurzor zůstal viditelný.

### HW Test 05-C2 — vložení po uvolnění místa: PASS

- Výchozí stav: **Length 255/256, Cursor 255, View offset 177**.
- Akce: vložit jeden ASCII znak `X`.
- Pozorovaný stav: **Length 256/256, Cursor 256, View offset 177**.
- Znak `X` byl přijat a byl viditelný na konci řádku.

**Souhrn: HW Testy 05-A, 05-B, 05-C1 a 05-C2 všechny PASS.** Editor
v testovaných scénářích správně zvládá kapacitní limit 256 znaků, odmítne
další vložení a po uvolnění místa znovu přijímá vstup.
**Tato hardwarová pozorování nepředstavují formální důkaz paměťové
bezpečnosti.** Nepotvrzují všechny pozice editace ani uzavření celé M2.2-A.
Čekat na další instrukce.

## M2.2-A HW Test 06 — editace dlouhého řádku při aktivním scrollingu: PASS

Výsledky zaznamenány 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.
Terminál: PuTTY, 38400 baud, 8N1.

### HW Test 06-A — výchozí dlouhý řádek: PASS

- Zadáno 85 ASCII znaků `a`.
- Potvrzený stav: **Length 085/256, Cursor 085, View offset 006**.
- Kurzor viditelný, bez zalomení řádku.

### HW Test 06-B1 — pohyb vlevo: PASS

- Akce: desetkrát stisknout Left.
- Potvrzený stav: **Length 085/256, Cursor 075, View offset 006**.
- Kurzor zůstal viditelný.

### HW Test 06-B2 — vložení při scrollovaném viewportu: PASS

- Akce: vložit `X` na pozici kurzoru 75.
- Potvrzený stav: **Length 086/256, Cursor 076, View offset 006**.
- `X` je viditelné, kurzor bezprostředně za ním.

### HW Test 06-C1 — Backspace: PASS

- Akce: jednou stisknout Backspace.
- Potvrzený stav: **Length 085/256, Cursor 075, View offset 006**.
- `X` odstraněno, kurzor viditelný.

### HW Test 06-C2 — Delete: PASS

- Akce: jednou stisknout Delete.
- Potvrzený stav: **Length 084/256, Cursor 075, View offset 006**.
- Odstraněn jeden znak `a`.

**Souhrn: HW Testy 06-A, 06-B1, 06-B2, 06-C1 a 06-C2 všechny PASS.**
Editace a navigace uvnitř dlouhého řádku při aktivním horizontálním scrollingu
fungují správně v testovaných scénářích. Nejde o ověření všech pozic nebo
uzavření celé M2.2-A. Čekat na další instrukce.

## M2.2-A HW Test 07-A — ukončení přes Ctrl+Q: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.
Terminál: PuTTY, 38400 baud, 8N1.

- Výchozí stav: **Length 084/256, Cursor 075, View offset 006**.
- Akce: jednou stisknout Ctrl+Q.

Pozorované výsledky:

- Editor Kilo68k se ukončil.
- Řízení se vrátilo firmware rosco_m68k.
- Systém provedl warm boot.
- Nebylo pozorováno zamrznutí programu.

**Hodnocení: PASS. Warm boot je očekávané chování platformy po ukončení
programu, nikoli chyba.** Tento test sám nepotvrzuje funkčnost terminálového
vstupu a výstupu po warm bootu; následná reakce na Enter je zaznamenána
samostatně v HW Testu 07-B níže.
Nejde o samostatné ověření všech kroků obnovy terminálu ani uzavření celé
M2.2-A. Čekat na další instrukce.

## M2.2-A HW Test 07-B — reakce firmware po warm bootu: PASS

Výsledek zaznamenán 2026-11-12 na základě výslovného potvrzení uživatele.
Hardware: skutečné rosco_m68k, CPU MC68010, 5 MB fyzické RAM.
Terminál: PuTTY, COM6, 38400 baud, 8N1.

- Výchozí podmínka: Kilo68k bylo ukončeno pomocí Ctrl+Q a rosco_m68k
  provedlo očekávaný warm boot.
- Akce: po warm bootu stisknout Enter v PuTTY.
- Pozorovaný výsledek: firmware rosco_m68k správně zareagovalo na Enter.

**Hodnocení: PASS pro ověřenou terminálovou interakci. HW Testy 07-A
a 07-B oba PASS.** Kilo68k se ukončí přes Ctrl+Q, platforma provede
očekávaný warm boot a sériový vstup firmware zůstává poté funkční,
jak potvrzuje reakce na Enter.

Tento výsledek neznamená, že byla nezávisle ověřena všechna nastavení
terminálu nebo všechny kroky jejich obnovy. Nejde o uzavření celé M2.2-A.
Čekat na další instrukce.

## Repozitáře a větev

- Pracovní repozitář: `/home/pavelk/rosco/rosco_m68k` (`~/rosco/rosco_m68k`).
- Fork / origin: `https://github.com/PavelKCZ/rosco_m68k`.
- Upstream rosco: `https://github.com/rosco-m68k/rosco_m68k`.
- Ověřená aktuální větev: `feature/kilo-editor`.
- HEAD: `b67ae1d38e48e4ff185305be3adad7315795265b`.
- Poslední commit: `feat(kilo): add hardware-validated M2.1 terminal prototype`.
- Lokální reference `origin/feature/kilo-editor` ukazuje na tentýž commit.
  Uživatel potvrdil dokončení push M2.1; při zápisu checkpointu nebyl
  kontaktován vzdálený server.
- `develop` je integrační baseline; neměnit ani do něj neslučovat.
- Původní antirez Kilo je samostatně v `/home/pavelk/rosco/kilo-upstream`,
  analyzovaný commit `323d93b29bd89a2cb446de90c4ed4fea1764176e`.
  Původní checkout se nemění.
- Před pokračováním přečíst `AGENTS.md`, `docs/KILO_PORTING_CONTEXT.md`,
  tento checkpoint a README obou aplikací. Komunikovat česky.

## Hotové milníky

### libtermios 1.0

Stabilní baseline v `code/software/libs/src/termios/`. **78/78 scénářů
prošlo na skutečném MC68010 hardware.** Podporuje kanonický i nekanonický
vstup a ověřené chování VMIN/VTIME. Jde o softwarové handles nad firmware
character-device API, nikoli Unixové deskriptory. Neměnit knihovnu kvůli
integraci editoru. Toto ověření neprokazuje správnost nového editoru.

### M2.1 — sériový terminálový prototyp

Dokončen, sestaven, uživatelem hardwarově validován a commitnut jako
`b67ae1d` (úplný hash výše). Aplikace: `code/software/kilo_terminal_test/`.
Potvrzeno: vykreslení, běžné znaky a hex výpis, šipky, speciální klávesy
(bez jednotlivého inventáře), Ctrl+A/B/C/P jako 01/02/03/10, q/Q jako 71/51,
ukončení skutečným bajtem 11 přes Tera Term a Ctrl+Q v PuTTY. Po normálním
ukončení následovalo očekávané chování firmware/resetu. Detaily jsou v README
M2.1. M2.1 během M2.2-A ani ukládání checkpointu nebylo měněno.

## Schválená architektura M2.2

- Samostatná aplikace `code/software/kilo68k/`; M2.1 zůstává referenční test.
- Zachovat koncepty Kilo: textové řádky, explicitní délky, logický kurzor,
  viewport a ANSI vykreslování. Nepřebírat upstream nezměněný.
- Oddělit malý terminálový modul od editoru; hlavní smyčka má společný cleanup.
- Bez curses, Unix ioctl, signálů, atexit a předpokladu plného POSIX runtime.
- Statická paměť a omezené buffery; nepřidávat heap jen kvůli větší RAM.
- Schválená a implementovaná etapa **M2.2-A je pouze jeden řádek / 256 znaků**.
- Architektonický plán následně uvažoval víceřádkovou etapu B (64×256),
  navigaci C a odolnost/uzavření D. Tyto další etapy nejsou nyní povolením
  k implementaci. Nezahajovat je bez nového schválení.
- Žádné otevření/ukládání souborů ani FAT/MicroSD v M2.2-A. Budoucí ukládání
  potřebuje vlastní analýzu write safety: existující FAT flush/close
  nepropagují všechny chyby a upstream truncate+write není bezpečnostní záruka.
- Upstream má nekontrolované malloc/realloc, závislost na memmove a vady
  aktualizace délky/renderu či posunu kurzoru. Adaptace se jim vyhýbá;
  stabilní knihovny se neopravují ani nepřepisují v rámci editoru.
- Upstream BSD 2-Clause copyright, podmínky a disclaimer jsou zachovány
  v aplikačním LICENSE; přikládat také k distribuovaným binárkám.

## Potvrzený hardware a paměť

Uživatel výslovně potvrdil: **základní konfigurace 1 MB fyzické RAM,
rozšířená 5 MB; aktuální testovací MC68010 deska má 5 MB**. Nejde o odhad
z linkerového skriptu. Kapacita fyzické RAM není velikost paměti dostupné
aplikaci. Pro budoucí větší alokace prohlédnout firmware report, linker,
startup, stack a mapu.

Aktuální build používá existující sériový linkerový skript
`libs/build/lib/ld/serial/hugerom_rosco_m68k_program.ld`, základní RAM oblast
1 MB, load adresu 0x40000 a run adresu 0x2000. Startup nastaví stack podle
SDB_MEMSIZE. Samotný symbol _STACK_SIZE není kontrolou kolize stacku;
bez libheap není aktivována jeho 32KiB rezerva. Rozšířených 5 MB skript
automaticky nevystavuje všem sekcím aplikace. Prototyp je malý a statický;
praktická kompatibilita s 1 MB se zachovává, ale nebyla na desce otestována.

RAM fakta jsou zaznamenána zde a v README nové aplikace. Původní
`docs/KILO_PORTING_CONTEXT.md` v této relaci zůstává beze změny.

## Současná implementace M2.2-A

Všech devět nových aplikačních souborů je na disku, zatím nesledovaných Gitem:

| Soubor pod `code/software/kilo68k/` | Účel |
| --- | --- |
| `main.c` | Statický dokument, hlavní smyčka, jediná cesta cleanup |
| `terminal.c` | Inicializace/obnova libtermios, parser, kontrolované zápisy |
| `terminal.h` | Terminálové API a kódy kláves |
| `editor.c` | Editační operace, viewport, bounded ANSI frame |
| `editor.h` | Stav editoru, délka a limity |
| `Makefile` | Existující build konvence, CPU/ARCH/TUNE 68010 |
| `.gitignore` | Ignoruje o/elf/bin/dis/map/sym |
| `README.md` | Funkčnost, limity, build, host testy a širší HW postup |
| `LICENSE` | Původní upstream BSD 2-Clause licence |

Funkčnost:

- Jeden řádek: nejvýše 256 tisknutelných ASCII znaků 20–7E, pole 257 B.
- Explicitní length/cursor/offset; NUL zachován. Plný řádek odmítá vložení
  bez změny textu, délky nebo kurzoru a zobrazí FULL.
- Left/Right, Home/End, Backspace (08 i 7F) a dopředný Delete.
- Enter, Tab, Up/Down, ostatní controls a ne-ASCII se nevkládají.
- Pevně 80×25, editace na řádku 5, horizontální scrolling; stav 7–8,
  help 24, RAM-only varování 25. Viditelný kurzor během čekání.
- Žádný redraw při timeoutu. Statický výstupní frame má 384 B.
- Ctrl+Q (11) ihned ukončí aplikaci a zahodí text; 1C není náhradní Ctrl+Q.
- Bind handle 0 → firmware device 0; uložit atributy, cfmakeraw,
  VMIN=0/VTIME=1, TCSANOW. Interaktivní I/O pouze rosco_termios_read/write.
- Kontrola částečných/nulových/chybových zápisů; cleanup i při chybách
  zkusí kurzor, obnovu uložených atributů a unbind. Neukládá obrazovku.
- Bez dynamické alokace, memmove, velkých automatických polí a souborového I/O.

## Poslední build a hostitelské ověření

Provedeno před checkpointem; při ukládání checkpointu se nic nepřestavovalo:

```sh
make -C code/software/kilo68k clean
make -C code/software/kilo68k CPU=68010
```

Oba příkazy skončily 0. m68k-elf-rosco GCC 13.3.1 (20240927), finální
kompilace/linkování bez diagnostik. Přilinkovány existující knihovny,
lokální EXTRA_LIBS = `-ltermios -lm` (__errno); knihovny se nepřestavují.
První pokus zachytil nerozpoznaný fall-through komentář v Backspace větvi;
opraven komentář bez změny logiky, následný čistý build prošel.

| Položka | Poslední velikost |
| --- | ---: |
| `kilo68k.bin` | 6132 B |
| `kilo68k.elf`, včetně debug informací | 48900 B |
| text | 6118 B |
| data | 4 B |
| BSS | 16412 B |

`_end = 0x7810`. Velká část BSS je startup a libtermios. ELF bez nevyřešených
symbolů, bez heap/FAT/memmove. BIN porovnán s ELF load segmenty: shoduje se.
SHA-256 existujícího BIN:
`1f567bc9ff638705d0cc6fe7e7c605f4deb7c86af99abc35ee92f456a94e0535`.
Build artefakty jsou na disku a ignorované; nesmazat je při obnovení relace.

Dočasný hostitelský harness `/tmp/kilo68k-review/check.c` s mock termios API
testoval skutečné aplikační zdroje. **Po restartu nemusí /tmp existovat**;
na harness ani jeho binárku nelze spoléhat jako na trvalé projektové soubory.
Zaznamenaný výsledek před checkpointem:

- Prošly všechny délky 0–256 a všechny pozice kurzoru pro vložení, Backspace
  a Delete proti referenčnímu textu; včetně hranic 0, 1, 255, 256 a FULL.
- Prošly kontroly NUL, délky/kurzoru/offsetu, scrollingu a výstupní pozice.
- Prošly CSI/SS3, neúplné, neznámé, poškozené a dlouhé sekvence, opakované
  ESC, timeouty, Ctrl+Q při zotavení a ignorování 1C/CR/Tab.
- Prošly simulované chyby bind/getattr/setattr/read/write/restore/unbind,
  částečné/nulové zápisy a absence redraw při timeoutu.
- AddressSanitizer a UBSan bez nálezů. První běh nemohl dokončit
  LeakSanitizer kvůli ptrace prostředí; opakovaný běh s
  `ASAN_OPTIONS=detect_leaks=0` prošel. Nelze tvrdit, že LeakSanitizer prošel.
- Statická revize nenašla blokující závadu. Tyto výsledky nejsou hardwarové
  testy ani záruka správnosti na MC68010.

## Známá omezení terminálu

- Tera Term 5.6.1: uživatel v M2.1 při Ctrl+Q pozoroval 1C místo 11,
  při českém i US Windows rozložení a flow control None. Přesná příčina
  v terminálu nebo konfiguraci není známá; nepřipisovat konkrétní komponentě.
  PuTTY Ctrl+Q fungovalo; odeslaný binární bajt 11 přes Tera Term také.
  Pro první M2.2-A test používat PuTTY. Žádný workaround ani úprava libtermios.
- Parser má 16 B za introducerem, jedno časované čtení na volání.
  Po neúplné/poškozené/přerostlé CSI/SS3 zahazuje do finálního bajtu 40–7E
  nebo nového ESC. Může proto spotřebovat první následující běžné písmeno
  jako zakončení. ESC s prodlevou alespoň 0,1 s obnoví normální vstup.
- Další ESC restartuje parser; Ctrl+Q má prioritu ve všech stavech.
  Jiné ESC rodiny se zahazují do idle prodlevy; pozdější bajty jsou nový vstup.
- Bez podpory modifikovaných kláves, Alt, UTF-8, OSC/DCS a bracketed paste.
- VTIME závisí na běžícím firmware 100Hz časovači. Propustnost rychlého paste
  a odezva na skutečné desce nebyly změřeny.
- Obnovují se softwarové atributy po bind a viditelnost kurzoru, nikoli UART
  konfigurace, Windows nastavení nebo původní obsah obrazovky. Při resetu,
  výpadku napájení nebo poruše firmware I/O není cleanup garantován.

## Git stav a omezení pro navazující relaci

Při zápisu checkpointu nejsou žádné modifikace sledovaných souborů ani staged
změny. Nesledovaných je výše uvedených devět aplikačních souborů a nově
`docs/KILO_DEVELOPMENT_CHECKPOINT.md`. Změny jsou **pouze lokální**, M2.2-A
ani tento checkpoint nejsou commitnuté ani pushnuté. Ignorované artefakty
zůstávají na disku. Před další prací znovu ověřit větev a `git status`;
nepřepsat případné nové uživatelské změny.

- Bez výslovného schválení nestagovat, necommitovat, nepushovat, nemergovat,
  neresetovat a nepřepisovat historii. Nikdy nepushovat do upstream repozitáře.
- Neměnit M2.1, libtermios, firmware, stabilní knihovny nebo globální build
  bez samostatného výslovného souhlasu; implementaci M2.2-A nyní zachovat.
- Nemazat zdroje, zálohy ani generované soubory.
- Neimplementovat více řádků, souborové operace, FAT zápisy, highlighting ani
  hledání. HW Testy 01, 02-A, 02-B, 03, 04-A, 04-B, 04-C, 05-A, 05-B, 05-C1 a 05-C2
  a dále 06-A, 06-B1, 06-B2, 06-C1, 06-C2, 07-A a 07-B jsou zaznamenány jako PASS;
  čekat na další instrukce.
- Hardwarové testy provádí uživatel. Neoznačovat neprovedené scénáře PASS.

Dokument byl vytvořen jako jediná změna této checkpointové operace;
`AGENTS.md`, původní kontext, aplikace i jejich artefakty zůstaly beze změny.
