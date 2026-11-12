# Kilo68k M2.2-A — jeden editovatelný řádek v RAM

Samostatná aplikace pro MC68010. Koncept řádku, kurzoru a ANSI vykreslování
vychází z antirez Kilo; terminálové rozhraní z ověřeného prototypu M2.1.
Upstream reference: `~/rosco/kilo-upstream`, commit
`323d93b29bd89a2cb446de90c4ed4fea1764176e`. BSD 2-Clause copyright,
podmínky a disclaimer jsou zachovány v LICENSE; přikládejte jej i k binární
distribuci. Původní checkout ani prototyp M2.1 se nemění.

## Funkčnost a struktura

- Přesně jeden řádek, nejvýše 256 tisknutelných ASCII znaků (0x20–0x7E).
  Statické pole má 257 bajtů; explicitní délka nezahrnuje konečné NUL.
- Left/Right, Home/End, Backspace (BS i DEL) a dopředný Delete.
  Enter, Tab, Up/Down, jiné řídicí znaky a ne-ASCII bajty se nevkládají.
- Pevná obrazovka 80×25. Text na řádku 5; stav na 7–8; nápověda na 24;
  trvalé varování RAM ONLY na 25. Horizontální offset udržuje kurzor v 1–80,
  včetně prázdné pozice za posledním znakem. Kurzor je při čekání viditelný.
- Plný buffer odmítne vložení beze změny textu, délky a kurzoru. První
  odmítnutí zobrazí FULL; opakované odmítnutí již nepřekresluje.
- Ctrl+Q (`0x11`) ihned ukončí aplikaci a zahodí text. Žádné ukládání,
  více řádků, hledání, highlighting, heap ani FAT/MicroSD operace.
- `main.c`: statický dokument, smyčka a jediná cleanup cesta.
  `editor.c/.h`: bajtové posuny, invarianty, viewport a 384bajtový frame.
  `terminal.c/.h`: životní cyklus libtermios, parser a kontrolované zápisy.

Invarianty: `length <= 256`, `cursor <= length`, `offset <= cursor`,
`cursor - offset < 80`, `text[length] == '\0'`. Vložení posouvá bajty
odzadu včetně NUL; mazání dopředu včetně NUL. Žádná závislost na memmove
ani přetypování znakových adres na word/long. Výstupní frame se nejprve
celý sestaví s kontrolou kapacity; potom se odešle. Žádná velká lokální pole.

## Terminál a parser

`rosco_termios_bind(0, 0)`, tcgetattr, cfmakeraw, VMIN=0, VTIME=1,
tcsetattr(TCSANOW). Interaktivní I/O výhradně přes rosco_termios_read/write.
Každé volání parseru čte nejvýše jeden bajt s timeoutem přibližně 0,1 s
(vyžaduje běžící firmware 100Hz časovač). Timeout není znak a nezpůsobuje
redraw. Kontrolují se i částečné, nulové a chybové zápisy.

Podporované sekvence: CSI/SS3 `A/B/C/D/H/F` (Up/Down jsou dekódovány,
ale editor je ignoruje); CSI `1~/7~` Home, `4~/8~` End, `3~` Delete.
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

**Tera Term 5.6.1:** v M2.1 uživatel pozoroval při Ctrl+Q hodnotu `0x1C`
místo `0x11`, s českým i US rozložením a flow control None. Přesná příčina
nebyla zjištěna. PuTTY odesílá funkční Ctrl+Q; přes Tera Term fungovalo
odeslání binárního souboru s jediným bajtem `0x11`. Pro validaci Ctrl+Q
doporučujeme PuTTY. V aplikaci ani libtermios není obcházející úprava.
Při HW Testu 02 se Delete v Tera Termu chovalo jako Backspace; přesné
odeslané bajty nebyly zachyceny. Rozdíly klávesnice zůstávají nevyřešené;
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

## Hostitelské ověření

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

## Hardwarová validace M2.2-A

Editor dokončil **18/18 plánovaných scénářů na skutečném hardware
(HW 01–07 včetně podtestů), všechny PASS**, podle výsledků potvrzených
uživatelem. Testované prostředí: skutečné rosco_m68k, CPU MC68010,
5 MB RAM, PuTTY, 38400 baud, 8N1.

Autoritativním záznamem podrobných postupů a pozorovaných výsledků je
[docs/KILO_DEVELOPMENT_CHECKPOINT.md](../../../docs/KILO_DEVELOPMENT_CHECKPOINT.md).
PASS se vztahuje na zaznamenané scénáře, nikoli na všechny kombinace vstupů.
Warm boot po ukončení Ctrl+Q je očekávané chování platformy; následná reakce
firmware na Enter byla potvrzena. Všechna nastavení terminálu nebyla
nezávisle ověřena. Hardwarové testy nejsou formálním důkazem paměťové bezpečnosti.

Zbývající omezení: poškozené/neúplné ESC sekvence a chybové cesty nebyly
komplexně hardwarově otestovány; konfigurace s 1 MB RAM ani propustnost
rychlého vkládání (fast paste) nebyly hardwarově otestovány. Rozdíly chování
kláves v Tera Termu zůstávají nevyřešené. Rozsah aplikace zůstává jeden
RAM-only ASCII řádek do 256 znaků, bez filesystemu, souborového I/O
a dynamické alokace paměti.

## Postup pro opakování a rozšíření hardwarových testů

Následující širší postup není totožný se seznamem 18 dokončených scénářů;
obsahuje také dosud neověřené varianty. Skutečně potvrzený rozsah určuje
výše odkazovaný checkpoint.

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
