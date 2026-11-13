# Kilo68k M2.2-B1 — F3-I1 hardware-validated

**F3-I1: hardware-validated, 12/12 scénářů PASS na skutečném rosco_m68k
MC68010**, potvrzeno uživatelem 2026-11-13. Celý milník M2.2-B1 tím není
automaticky označen za uzavřený.

F3 obsluhuje RX také při sestavování ANSI rámce, nadále používá F1 FIFO
512 B a bounded pumpu nejvýše 16 čtení. D2 diagnostika zůstává pod
`KILO_RX_DIAGNOSTICS`; Ctrl+R resetuje, Ctrl+T zmrazí report, Ctrl+Q ukončí.

Potvrzené scénáře: Python 2 ms, Python 1 ms a jeho opakování (vše 100/100),
rychlý Paste v Tera Term i PuTTY (oba 100/100), autorepeat dolů i nahoru
10 s, Backspace/DELETE s výsledkem `AC`, editace uprostřed s výsledkem
`ABCDXE`, víceřádková editace, scrolling přes 23řádkovou hranici a vizuální
integrita 100 znaků s očekávanou diagnostikou. Podrobný inventář je v
[docs/KILO_F3_CHECKPOINT.md](../../../docs/KILO_F3_CHECKPOINT.md).

Před F3: PuTTY Paste 59/100 a Python 1 ms dvakrát 96/100. Po F3 nebyla v
provedených scénářích pozorována ztráta znaků. **UART overrun nebyl přímo
změřen; F3 nezaručuje bezchybný přenos při libovolném zatížení.** Testy nejsou
formálním důkazem paměťové bezpečnosti. Tera Term je nyní preferovaný;
DELETE a Ctrl+Q jsou nakonfigurovány na stejné kódy jako v PuTTY.
Host testy, ASan/UBSan a buildy již dříve prošly; tato aktualizace je pouze
dokumentační, nic se znovu nesestavovalo ani netestovalo. Další krok čeká na
výslovné instrukce; F2 se nezahajuje.

Níže jsou zachovány i historické buildy a výsledky předchozích etap.
Aktuální velikosti F3 jsou v checkpointu: běžný BIN 7988 B, diagnostický
`kilo68k-rxdiag.bin` 10416 B.
Renderer zobrazuje dokumentový viewport 23×80, status na řádku 24 a trvalé
RAM-only varování na řádku 25. První zobrazení i další refreshe používají
stejný layout; preview titulek a hlášky byly odstraněny.
B1.1 a B1.2 zůstávají zachovány: editační sémantika se nemění.
Výsledky 18/18 níže patří
výhradně předchozí baseline M2.2-A, commit `e2e8db67c6a7da6b66b0be1a8cf51d175189409c`.

Samostatná aplikace pro MC68010. Koncept řádku, kurzoru a ANSI vykreslování
vychází z antirez Kilo; terminálové rozhraní z ověřeného prototypu M2.1.
Upstream reference: `~/rosco/kilo-upstream`, commit
`323d93b29bd89a2cb446de90c4ed4fea1764176e`. BSD 2-Clause copyright,
podmínky a disclaimer jsou zachovány v LICENSE; přikládejte jej i k binární
distribuci. Původní checkout ani prototyp M2.1 se nemění.

## Funkčnost a struktura

- Pevné pole 64 řádků, každý nejvýše 256 tisknutelných ASCII znaků (0x20–0x7E).
  Každé znakové pole má 257 bajtů; explicitní uint16_t délka nezahrnuje NUL.
  Inicializace vytvoří jeden prázdný řádek a vynuluje délky a první bajty
  všech slotů, nikoli jejich nevyužitý obsah. Dokument nikdy nemá nula řádků.
- Enter rozdělí aktuální řádek v cx: vlevo původní řádek, vpravo nový řádek,
  cy se zvýší a cx přejde na nulu. Následující řádky se posouvají odzadu.
  Při 64 řádcích se Enter odmítne atomicky s hláškou DOCUMENT FULL.
- Left/Right, Home/End, Backspace (BS i DEL) a dopředný Delete.
  Platí pouze uvnitř aktuálního řádku. Na hranicích jsou Left/Right/BS/Delete
  no-op: žádné spojování a žádné přechody Left/Right mezi řádky. Tab, jiné
  řídicí znaky a ne-ASCII bajty se nevkládají.
- Up/Down mění cy o jeden v mezích dokumentu a nastaví
  `cx = min(původní cx, délka cílového řádku)`. Preferovaný sloupec se
  neuchovává. Na prázdném řádku je cx=0. Up na prvním a Down na posledním
  řádku jsou no-op bez změny dokumentu, kurzoru, offsetů, diagnostiky i redraw.
- Pevná obrazovka 80×25. Fyzické řádky 1–23 obsahují dokumentové řádky
  rowoff až rowoff+22. Řádky za koncem dokumentu jsou prázdné.
  Status na řádku 24 má formát
  `Row 001/001 Len 000/256 Col 000 Off 000,000 | READY`, kde Off je rowoff,coloff.
  Diagnostika je READY, LINE FULL nebo DOCUMENT FULL. Status se nezkracuje.
  Řádek 25 obsahuje `RAM ONLY - Ctrl-Q exits immediately and discards all text.`
  (58 znaků). Každý fyzický řádek je před výpisem celý vymazán.
- Horizontální offset platí pro všechny viditelné řádky. Při length <= coloff
  se nevypisuje nic a nevytváří se ukazatel text+coloff; jinak se vypíše
  nejvýše 80 znaků. Kurzoru odpovídá fyzická pozice
  `(cy-rowoff+1, cx-coloff+1)`, kontrolovaná na rozsah 1–23 a 1–80.
  Kurzor je skrytý během refresh a viditelný po jeho dokončení.
- Plný buffer odmítne vložení beze změny textu, délky a kurzoru. První
  odmítnutí zobrazí LINE FULL; opakované stejné odmítnutí již nepřekresluje.
  Odmítnutí mění pouze diagnostiku; dokument, kurzor i offsety zachová.
  Úspěšná změna nebo pohyb obnoví READY; no-op diagnostiku nemění.
- Ctrl+Q (`0x11`) ihned ukončí aplikaci a zahodí text. Žádné ukládání,
  hledání, highlighting, heap ani FAT/MicroSD operace.
- `main.c`: statický dokument, smyčka a jediná cleanup cesta.
  `editor.c/.h`: bajtové posuny, invarianty, viewport a statický frame 4096 B.
  `terminal.c/.h`: životní cyklus libtermios, parser a kontrolované zápisy.

Invarianty: `1 <= row_count <= 64`, `cy < row_count`,
`cx <= rows[cy].length <= 256`, `rows[i].text[rows[i].length] == '\0'`
pro každý aktivní řádek; ukládá se pouze tisknutelné ASCII.
`coloff <= cx`, `cx - coloff < 80`, `rowoff <= cy`, `cy - rowoff < 23`.
Vložení posouvá bajty
odzadu včetně NUL; mazání dopředu včetně NUL. Žádná závislost na memmove
ani přetypování znakových adres na word/long. Výstupní frame se nejprve
celý sestaví s kontrolou kapacity; potom se odešle. Žádná velká lokální pole.

## Terminál a parser

`rosco_termios_bind(0, 0)`, tcgetattr, cfmakeraw, VMIN=0, VTIME=1,
tcsetattr(TCSANOW). Interaktivní I/O výhradně přes rosco_termios_read/write.
Každé volání parseru čte nejvýše jeden bajt s timeoutem přibližně 0,1 s
(vyžaduje běžící firmware 100Hz časovač). Timeout není znak a nezpůsobuje
redraw. Kontrolují se i částečné, nulové a chybové zápisy.

**Enter:** uživatel pozoroval v PuTTY bajt CR (`0x0D`) existující M2.1
hardwarovou diagnostikou. B1.1 mapuje CR na KEY_ENTER pouze ve stavu INPUT;
LF (`0x0A`) ignoruje. CRLF tedy znamená jeden split, i s timeoutem mezi bajty.
Samostatné LF nemá efekt. CR uvnitř neúplné/zahazované ESC sekvence není Enter.

Podporované sekvence: CSI/SS3 `A/B/C/D/H/F` (Up/Down/Right/Left/Home/End);
CSI `1~/7~` Home, `4~/8~` End, `3~` Delete.
Samostatný Escape nic nemění. Další ESC restartuje parser; Ctrl+Q má
přednost ve všech stavech. `0x1C` se za Ctrl+Q nepovažuje.

Sekvenční buffer je omezený na 16 bajtů za introducerem. Neznámá dokončená
CSI/SS3 se zahodí. Po překročení limitu, neplatném bajtu nebo timeoutu
rozpracované CSI/SS3 se ignoruje pokračování až do prvního finálního bajtu
0x40–0x7E nebo nového ESC; timeouty stále vracejí řízení hlavní smyčce.
Tím se zbytky sekvence nevkládají do dokumentu. Cena tohoto konzervativního
chování: po neúplné sekvenci může být první běžné písmeno spotřebováno jako
její zakončení. Stisk samostatného ESC a vyčkání alespoň 0,1 s obnoví běžný
vstup. Jiné ESC rodiny se ignorují do první prodlevy bez dat nebo nového ESC.
Po této prodlevě už další bajty tvoří nový vstup, nikoli pokračování staré
rodiny. Modifikované klávesy, Alt, OSC/DCS a bracketed paste nejsou podporovány.

Cleanup zkusí zobrazit kurzor, obnovit uložené atributy, vypsat závěrečný
stav a uvolnit handle, i když předchozí operace selhala. Při selhání bind
není přes toto rozhraní kam vypsat chybu. Při selhání tcgetattr se neobnovuje
neinicializovaná struktura. Atributy jsou softwarové hodnoty po bind;
UART a Windows terminál se nekonfigurují. Obsah obrazovky se neobnovuje.
Při resetu, výpadku napájení nebo poruše firmware I/O nelze úklid zaručit.
Firmware startup návratový kód kmain ignoruje.

**Aktuální konfigurace (2026-11-13):** preferován Tera Term;
DELETE a Ctrl+Q nastaveny na stejné kódy jako v PuTTY. Následující odlišnosti
jsou historická pozorování, jejich původní příčina nebyla potvrzena.

**Historicky, Tera Term 5.6.1:** v M2.1 uživatel pozoroval při Ctrl+Q hodnotu `0x1C`
místo `0x11`, s českým i US rozložením a flow control None. Přesná příčina
nebyla zjištěna. PuTTY odesílá funkční Ctrl+Q; přes Tera Term fungovalo
odeslání binárního souboru s jediným bajtem `0x11`. Pro validaci Ctrl+Q
bylo tehdy doporučeno PuTTY. V aplikaci ani libtermios není obcházející úprava.
Při HW Testu 02 se Delete v Tera Termu chovalo jako Backspace; přesné
odeslané bajty nebyly zachyceny. Původní příčina rozdílů nebyla určena;
není potvrzena chyba editoru ani konkrétní příčina na straně terminálu.

## Build a paměť

Z kořene repozitáře:

```sh
make -C code/software/kilo68k clean
make -C code/software/kilo68k CPU=68010
```

Lokální Makefile vynucuje CPU/ARCH/TUNE 68010 a používá existující software.mk,
sériový startup a instalované knihovny; `-ltermios -lm` kvůli __errno.
Knihovny se nepřestavují. Jejich původní kompilační nastavení tento build
samostatně neověřuje. Výstup pro nahrání je `kilo68k.bin`.

Uživatel potvrdil fyzické konfigurace 1 MB a 5 MB; aktuální testovací deska
MC68010 má **5 MB**. Aplikace další RAM nevyužívá. Použitý linkerový skript
umisťuje program do základní 1MB oblasti, nikoli automaticky do celých 5 MB.
Load adresa je 0x40000, run adresa 0x2000; startup nastavuje stack podle
firmware SDB_MEMSIZE. Symbol _STACK_SIZE ve skriptu není sám o sobě kontrolou
kolize stacku. Bez libheap se neaktivuje jeho 32KiB rezerva. Budoucí dostupná
paměť musí být určena z firmware reportu, mapy, potřeb startupu a stacku,
nikoli jen z fyzické kapacity. Statická velikost tohoto prototypu umožňuje
zachovat praktickou kompatibilitu s 1 MB; na této konfiguraci nebyl testován.
Sdílený kontextový dokument se v této etapě neupravuje.

## Logika viewportu B1.2

Po úspěšné editaci nebo skutečném pohybu: při cy < rowoff nastav rowoff=cy,
jinak při cy-rowoff >= 23 nastav rowoff=cy-22. Při cx < coloff nastav
coloff=cx, jinak při cx-coloff >= 80 nastav coloff=cx-79. V ostatních
případech offsety neměň. Porovnání používají unsigned výpočet a odečtení
offsetu probíhá až po ošetření opačného pořadí souřadnic. Odmítnutí Enter
při 64 řádcích nadále mění pouze diagnostiku; no-op nemění ani diagnostiku.
Plný renderer nyní mapuje rowoff na fyzický řádek 1; na všechny viditelné
řádky aplikuje stejný coloff. Dřívější preview pouze rows[cy] již není aktivní.

## ANSI frame a jeho horní mez B1.3a

Použité sekvence: CUP, ESC[2K, ESC[0m, ESC[?25l a ESC[?25h.
Renderer nepotřebuje ESC[2J: každý refresh vymaže všech 25 řádků.
Nevysílá CR/LF, barvy, padding ani uživatelsky neomezené zprávy.
CUP(r,c) má délku `4 + digits(r) + digits(c)` bajtů. Souřadnice mají
jednu nebo dvě číslice; statusová čísla mají přesně tři číslice.

| Součást | Maximální délka v bajtech |
| --- | ---: |
| Reset atributů + skrytí kurzoru | 10 |
| CUP(s,1) pro s=1..23: 23×5 + 9 + 14×2 | 152 |
| Vymazání 23 řádků: 23×4 | 92 |
| Viditelný text: nejvýše 23×80 | 1840 |
| Status: CUP(24,1) + clear + nejvýše 59 znaků | 70 |
| Varování: CUP(25,1) + clear + 58 znaků | 69 |
| Závěrečný CUP (nejvýše 8 B) + zobrazení kurzoru | 14 |
| **Celkem** | **2247** |

Tato mez zahrnuje první i následný frame. Je menší než původní konzervativní
audit 2327 B. Rezerva 4096−2247 = 1849 B. Každý append má kontrolu kapacity;
před odesláním se navíc kontroluje dokázaná mez. Nic se tiše nezkracuje.
Celý frame se sestaví před terminal_write; chyba přípravy neodešle jeho část.
Viditelné řádky se kontrolují na délku/NUL a odesílané znaky na tisknutelné ASCII.

Po znaku ve sloupci 80 následuje absolutní CUP, které má zrušit pending
auto-wrap. Do buňky 25:80 se nikdy nepíše text. Reálné chování tohoto
VT režimu je pokryto výše uvedenými HW scénáři v jejich rozsahu; nejde o
záruku pro všechny terminály a režimy. Jeden write není
atomická změna obrazovky ani záruka fyzického TX dokončení.

## Historický build a hostitelské ověření B1.3a

Prošly všechny příkazy s kódem 0:

```sh
make -C code/software/kilo68k/tests test
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize
make -C code/software/kilo68k CPU=68010
```

Native testy kompilují produkční editor.c, terminal.c a main.c s mock
libtermios a samostatným testovacím ANSI interpretem `tests/ansi.c/.h`.
Interpret zpracovává stream i po jednotlivých bajtech, modeluje matici
25×80, kurzor, viditelnost, pending wrap a počítá wrap/scroll i zápisy do
25:80. Podporuje přesné používané sekvence a ED; neznámé, poškozené a
mimorozsahové CUP odmítá. Nové ESC při cleanup může zrušit přerušenou CSI.
Samostatné testy ověřují i chování samotného modelu.

Každý render se porovnává se samostatně vypočtenými **všemi 2000 buňkami**,
kurzorem a jeho viditelností. Ověřeny jsou počty řádků 1/5/23/24/64,
rowoff 0/1/41, první/poslední viditelný řádek, délky 0/1/79/80/81/256,
coloff 0/1/6/177, kratší sousední řádky, plný viewport, zkrácení a vyprázdnění
textu bez zbytků, změny diagnostik, opakované refreshe a trvalé varování.
Capture je omezený a interpret streamuje bez akumulace celé relace.

Největší pozorovaný produkční frame měl **2247 B**, přesně horní mez.
Zápisy omezené na 1/2/3 bajty prošly včetně rozdělených ANSI sekvencí.
Chyby před/uvnitř/na konci frame, nulové zápisy a chyby přípravy nevedly
k hostitelskému přetečení; cleanup zkusí obnovit kurzor, atributy i unbind.
Test také výslovně potvrzuje, že při selhání všech výstupů nelze viditelnost
kurzoru zaručit. Všechny B1.1/B1.2 regrese prošly s novým rendererem.

ASan/UBSan bez nálezů; LeakSanitizer byl vypnutý kvůli ptrace prostředí.
MC68010 GCC 13.3.1: bez varování/chyb a nevyřešených symbolů.
ELF 55712 B včetně debug informací, BIN 7260 B;
text 7238 B, data 4 B, BSS 36500 B, `_end = 0xCAF0`.
Struktury zůstávají cílově 260 B (editor_row) a 16652 B (editor).
Žádný heap, souborové I/O, změna knihoven ani linkeru.

V době tohoto historického B1.3a buildu ještě hardwarové testy neproběhly.
Následně byly uživatelem potvrzeny B1.3b H01–H11 (11/11 PASS) a F3 scénáře
výše (12/12 PASS). Tyto výsledky automaticky neuzavírají celý B1.
Latence a obecná sériová propustnost nejsou univerzálně garantovány. Největší frame trvá teoreticky asi 0,585 s
při 38400 baud a 8N1; rychlé psaní/paste stále nemá garantovanou propustnost.
Historické Tera Term rozdíly jsou popsány výše; LF-only vstup a zotavení
neúplných ESC zůstávají omezením.

## Historický build a ověření B1.2

Všechny tři příkazy prošly s návratovým kódem 0:

```sh
make -C code/software/kilo68k/tests test
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize
make -C code/software/kilo68k CPU=68010
```

Bez varování/chyb a bez nálezů ASan/UBSan; LeakSanitizer je vypnutý.
Nové host testy přidávají Up/Down na hranicích i mezi stejně dlouhými řádky,
všechny sloupce pro kombinace délek 0,1,79,80,81,255,256 v obou směrech,
clamping bez preferovaného sloupce, prázdné řádky, Home/End, rowoff při
cy=22/23 až cy=63 a zpět na cy=0, opakované no-op včetně zachování diagnostik,
Enter s následnou navigací a odmítnutí v plném dokumentu. Každá operace
ověřuje invarianty a preview výstup; pohyb navíc zachování celého dokumentu.
Main-loop test potvrzuje, že hraniční Up/Down neposílá zbytečný redraw.
Všechny dosavadní B1.1 testy zůstávají, kromě záměrně změněného očekávání,
že Up/Down mezi řádky byly no-op. Parser a cleanup mají regresní pokrytí.

Cílové velikosti B1.2: text 7064 B, data 4 B, BSS 32788 B.
ELF 53744 B včetně debug informací, BIN 7076 B, `_end = 0xBBB8`.
Struktury se nemění: cílový editor_row 260 B a editor 16652 B (měření B1.1).
Žádné nevyřešené symboly, nové knihovny, heap nebo souborové I/O.
Tehdejší rozsah B1.2 končil navigací a offsety bez plného rendereru.
Nyní je implementován B1.3a výše; hardwarová validace zůstává budoucí prací.

## Historický build a ověření B1.1

Příkaz `make -C code/software/kilo68k CPU=68010` prošel kompilací i linkováním
bez varování/chyb po opravě porovnání znaménkovosti offsetů uint16_t.
ELF 53744 B včetně debug informací, BIN 7080 B;
text 7058 B, data 4 B, BSS 32788 B, `_end = 0xBBBC`.
Z DWARF cílového editor.o: sizeof(struct editor_row) = 260 B,
sizeof(struct editor) = 16652 B. Nejde pouze o odhad podle hostitelského ABI.
Žádné nevyřešené symboly, heap ani FAT; původní linker a knihovny beze změny.

Reprodukovatelné native testy jsou v `tests/`, mimo firmware zdroje:

```sh
make -C code/software/kilo68k/tests test
make -C code/software/kilo68k/tests sanitize
```

Prostředí, v němž LeakSanitizer nefunguje pod ptrace, může použít:

```sh
ASAN_OPTIONS=detect_leaks=0 make -C code/software/kilo68k/tests sanitize
```

Host testy používají skutečné editor.c, terminal.c a main.c s mock termios
API a původní hlavičkou termios. Host-only libc a memmove patří pouze testům.
Ověřují každý split pro délky 0–256 a všechny pozice, pořadí následných řádků,
posun až do slotu 63, opakované atomické odmítnutí 65. řádku, existující
editaci ve všech pozicích a délkových mezích, offsety, NUL a ASCII invarianty
po každé operaci, bounded preview výstup, CR/LF/CRLF, parserové regrese,
Ctrl+Q, lifecycle a chyby I/O. Příprava přímých fixtures v testu není novou
aplikační navigací. V této historické etapě B1.1 byly Up/Down no-op;
v aktuální B1.2 je nahrazuje navigace popsaná výše.

Běžný host test i ASan/UBSan s detect_leaks=0 prošly. LeakSanitizer tímto
není ověřen. Hardwarové testy B1.1 nebyly provedeny; acceptance se odkládá
do dokončení plného B1 rendereru. Zde končí rozsah B1.1, bez dalších etap.

## Historické hostitelské ověření M2.2-A

GCC m68k-elf-rosco 13.3.1 (20240927): kompilace i linkování bez varování
a chyb po opravě označení záměrného fall-through v Backspace větvi.
ELF 48900 B včetně debug informací, BIN 6132 B;
text 6118 B, data 4 B, BSS 16412 B. `_end = 0x7810`.
Významná část BSS patří existujícímu startupu a libtermios.
ELF nemá nevyřešené symboly; aplikace nepřilinkovala heap ani FAT.

Dočasný hostitelský harness mimo repozitář (`/tmp/kilo68k-review/check.c`)
s náhradou termios API ověřil skutečné aplikační zdroje:

- Každou délku 0–256 a každou pozici kurzoru: vložení, Backspace a Delete
  proti referenčnímu textu, včetně zachování textu při zaplnění.
- NUL, rozsahy délky/kurzoru/offsetu, horizontální viewport a výstupní pozici
  kurzoru při každé takové operaci; postupné vymazání plného řádku.
- CSI/SS3, opakované ESC, timeouty, neznámé/poškozené/příliš dlouhé sekvence,
  Ctrl+Q při zotavení a ignorování `0x1C`, CR a Tab.
- Cleanup při selhání bind/getattr/setattr/read/write/restore/unbind,
  částečné a nulové zápisy a absenci redraw při idle timeoutu.

Výsledek: hostitelské kontroly prošly s AddressSanitizer a UBSan.
LeakSanitizer nemůže v tomto prostředí pod ptrace běžet; kontrola byla
opakována s `ASAN_OPTIONS=detect_leaks=0`. Harness není součást aplikace.
Tyto testy nenahrazují spuštění na skutečné MC68010 desce.

## Historická hardwarová validace M2.2-A

Předchozí jednorádkový editor M2.2-A dokončil **18/18 plánovaných scénářů na skutečném hardware
(HW 01–07 včetně podtestů), všechny PASS**, podle výsledků potvrzených
uživatelem. Testované prostředí: skutečné rosco_m68k, CPU MC68010,
5 MB RAM, PuTTY, 38400 baud, 8N1.

Autoritativním záznamem podrobných postupů a pozorovaných výsledků je
[docs/KILO_DEVELOPMENT_CHECKPOINT.md](../../../docs/KILO_DEVELOPMENT_CHECKPOINT.md).
PASS se vztahuje na zaznamenané scénáře, nikoli na všechny kombinace vstupů.
Warm boot po ukončení Ctrl+Q je očekávané chování platformy; následná reakce
firmware na Enter byla potvrzena. Všechna nastavení terminálu nebyla
nezávisle ověřena. Hardwarové testy nejsou formálním důkazem paměťové bezpečnosti.

Historická omezení při uzavření M2.2-A: poškozené/neúplné ESC sekvence a chybové cesty nebyly
komplexně hardwarově otestovány; konfigurace s 1 MB RAM ani propustnost
rychlého vkládání (fast paste) nebyly hardwarově otestovány. Rozdíly chování
kláves v Tera Termu zůstávají nevyřešené. Rozsah aplikace zůstává jeden
RAM-only ASCII řádek do 256 znaků, bez filesystemu, souborového I/O
a dynamické alokace paměti.
Tento odstavec popisuje historickou baseline A, nikoli aktuální model B1.1.

## Historický postup pro opakování a rozšíření testů M2.2-A

Následující širší postup není totožný se seznamem 18 dokončených scénářů;
obsahuje také dosud neověřené varianty. Skutečně potvrzený rozsah určuje
výše odkazovaný checkpoint.
Postup není přejímacím plánem B1.3a: od B1.1 se změnil status a Enter
dělí dokument; B1.3a nově používá plný renderer. Jeho hardwarové testy
v této historické etapě ještě nebyly provedeny. Aktuální F3 výsledky jsou
uvedeny v úvodu a F3 checkpointu; následující starý postup není aktuálním
pokynem k testování.

1. Na potvrzené 5MB desce MC68010 použijte Windows, COM6, 38400 baud,
   8N1, flow control None, lokální echo vypnuté, terminál 80×25.
   Pro první test použijte PuTTY. Nahrajte BIN obvyklým postupem rosco_m68k.
2. Ověřte titulek, trvalé RAM ONLY varování, prázdný řádek 5,
   délku/kurzor/offset 000 a viditelný kurzor ve sloupci 1. Několik sekund
   nepište: žádné blikání či periodické překreslování.
3. Napište `abcd`, dvakrát Left a `X`: očekávejte `abXcd` a cursor 003.
   Backspace vrátí `abcd` a cursor 002; Delete dá `abd` a cursor 002.
   Home, Backspace: beze změny. End, Delete: beze změny. Home, Delete
   dá `bd`. End a dva Backspace vrátí prázdný řádek.
4. Stiskněte Enter, Tab, Up/Down, Ctrl+A/B/C/P, q a Q. Pouze q a Q
   se mají vložit jako text. Vymažte je Backspace. Volitelně pošlete `0x1C`:
   aplikace nesmí skončit ani změnit text.
5. Zadejte vzor `0123456789abcdef` šestnáctkrát, celkem přesně 256 znaků.
   Při pomalém vkládání sledujte přechody délek 079/080/081 a 255/256;
   kurzor nesmí opustit řádek 5. Na konci: délka 256, cursor 256,
   offset 177, fyzický sloupec 80. Další znak musí zobrazit FULL,
   ale zachovat text a souřadnice. Totéž zkuste při Home a uprostřed řádku.
6. End, Backspace: délka 255. Vložte původní poslední znak `f`:
   délka opět 256. Projeďte celý text Left/Right a Home/End; ověřte
   původní opakovaný vzor a scrolling v obou směrech. Opakovaným Delete
   od Home vymažte řádek; na nule další mazání nesmí nic poškodit.
7. Pošlete doslovné bajty sekvencí (například terminálovým makrem/sériovým
   nástrojem): `1B 5B 39 7E` (neznámá CSI), `1B 1B 5B 44` (opakovaný ESC),
   ESC + `[` + 32 číslic `1` + `~` (překročení limitu). Číslice ani `~`
   se nesmějí vložit. U neúplné `1B 5B` vyčkejte >0,1 s, poté pošlete
   `33 7E`; oba bajty se zahodí. Následné `x` se má vložit normálně.
   Samostatný ESC s prodlevou nemá měnit dokument. Ověřte také SS3 Left/Right
   `1B 4F 44` / `1B 4F 43` a CSI Home/End/Delete dle mapování výše.
8. Ctrl+Q v PuTTY musí ihned ukončit aplikaci, zobrazit kurzor a závěrečný
   text, pak vrátit očekávané chování firmware. Opakujte spuštění a ukončete
   také po neúplné CSI: `1B 5B`, prodleva, `11`.
9. Podle potřeby opakujte v Tera Term 5.6.1; anomálii klávesy Ctrl+Q
   zaznamenejte odděleně a ukončení ověřte odesláním skutečného bajtu `11`.
   Zapište každý výsledek, terminál, zjištěné odchylky a odezvu při psaní.

Omezení: rychlé hromadné vkládání nemá zaručenou propustnost; výkon se zatím
neměřil. Nejde o víceřádkový editor ani terminál s vyjednáváním velikosti.
Zaznamenaných 18/18 scénářů M2.2-A prošlo na desce; samotný úspěšný build
a hostitelské testy nenahrazují další hardwarová ověření mimo tento rozsah.
