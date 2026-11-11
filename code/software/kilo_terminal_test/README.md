# Kilo68k M2.1 — samostatný test terminálu

Test pro MC68010, bez editoru, dynamické alokace a přístupu k MicroSD/FAT.
Používá existující `software.mk`, instalované knihovny a sériový startup
s bodem vstupu `kmain`. CPU, ARCH i TUNE jsou lokálně pevně 68010.
Žádná knihovna se tímto Makefilem nesestavuje ani neinstaluje.

## Sestavení

Z kořene repozitáře:

```sh
make -C code/software/kilo_terminal_test CPU=68010
```

Výstup pro nahrání: `kilo_terminal_test.bin`. Ostatní výstupy standardního
build systému jsou ignorovány lokálním `.gitignore`.

## Ruční postup na desce

Reprodukovatelný postup; rozsah skutečně potvrzených výsledků uvádí sekce
hardwarové validace níže. Ne všechny zde navržené scénáře mají záznam výsledku.

1. Připravte Tera Term: COM6, 38400 baud, 8N1, bez lokálního echa,
   ANSI/VT100 kompatibilní terminál alespoň 80×25. Firmware musí poskytovat
   znakové zařízení 0 a běžící 100Hz časovač.
2. Nahrajte a spusťte binární soubor obvyklým postupem pro rosco_m68k.
3. Ověřte obrazovku a absenci automatického echa. Stiskněte písmena, čísla,
   Enter, Tab, Backspace (BS i DEL podle konfigurace terminálu).
   Řádky 7–8 zobrazují název klávesy a přijaté bajty hexadecimálně.
4. Ověřte šipky: X se pohybuje v oblasti řádků 10–23 a sloupců 1–80.
   Home/End přesunují na sloupec 1/80, PageUp/PageDown na řádek 10/23.
   Delete a Backspace se pouze zobrazují, nic nemažou.
5. Samostatný Escape se má zobrazit po přibližně 0,1 s. Pošlete také
   neúplné `ESC [` a neznámé `ESC [ 9 ~`; aplikace má pokračovat.
   Pro limit délky lze poslat ESC, znak `[` a 14 číslic bez zakončení.
6. Ctrl-Q má ukončit test, zobrazit kurzor a obnovit uložené atributy.
   Ověřte následné chování konzole. Ctrl-Q funguje i uvnitř rozpracované
   escape sekvence. Vypněte případné zachytávání Ctrl-Q na hostiteli.

## Dekódování a meze

- Podpora `ESC [ A/B/C/D/H/F` i `ESC O A/B/C/D/H/F`.
- Home: `ESC [ 1 ~`, `ESC [ 7 ~`; End: `ESC [ 4 ~`, `ESC [ 8 ~`.
- Delete: `ESC [ 3 ~`; PageUp/PageDown: `ESC [ 5 ~` / `ESC [ 6 ~`.
- Backspace: 0x08 nebo 0x7F. Ostatní bajty se zobrazují hexadecimálně;
  tisknutelné ASCII také jako znak. Vstup se nikdy neposílá přímo jako ANSI.
- Čtení používá výhradně `rosco_termios_read(0, ..., 1)` s VMIN=0,
  VTIME=1. Každé čekání na pokračování má timeout asi 0,1 s; sekvence má
  nejvýše 16 bajtů včetně ESC. Neúplná sekvence se označí při timeoutu,
  neznámá při zakončení nebo neplatném bajtu, příliš dlouhá při limitu.
  Zbytek po limitu se zpracuje jako nový vstup. Pomalé sekvence se mohou
  rozdělit; modifikované klávesy, Alt, UTF-8 a vložení bloků nejsou dekódovány.
- Obrazovka je pevná 80×25, bez dotazu na rozměry. Překreslují se jen
  informační řádky a X; výkon na desce nebyl změřen.
- Handle 0 se nejprve naváže na firmware device 0. Uložené atributy jsou
  softwarové výchozí hodnoty po bind, nikoli nastavení hostitelského terminálu.
  UART ani rychlost se nemění. Výstup používá výhradně rosco_termios_write;
  obsluha kontroluje chyby i částečné zápisy.
- Normální konec i chyba po získání atributů zkusí zobrazit kurzor a obnovit
  atributy přes TCSANOW; následně uvolní handle. Selhání bind nemůže vypsat
  zprávu tímto I/O. Návratový kód kmain firmware startup ignoruje.
  Při poruše výstupu nelze zaručit doručení obnovy kurzoru. Při resetu nebo
  výpadku napájení se úklid nespustí. Původní obsah obrazovky se neobnovuje.

## Záznam ověření

Host: m68k-elf-rosco GCC 13.3.1 (20240927). První pokus prošel kompilací,
ale linkování selhalo kvůli vstupu `main`; opraveno na `kmain`.
Další linkování odhalilo závislost libtermios na `__errno`; poskytuje ji
existující libm, přidaná do lokálního EXTRA_LIBS za libtermios.
Výše uvedený příkaz poté dokončil kompilaci i linkování s návratovým kódem 0,
bez diagnostik překladače/linkeru. ELF: text 5154 B, data 4 B, BSS 15684 B
(včetně existujícího startupu a libtermios). Vznikl BIN i disassembly.
Při přípravě uzavření M2.1 byly z kořene repozitáře znovu provedeny
`make -C code/software/kilo_terminal_test clean` a
`make -C code/software/kilo_terminal_test CPU=68010`: oba skončily kódem 0,
bez varování či chyb, se stejnými velikostmi sekcí. BIN má 5176 bajtů.
Build používá již instalované knihovny; nepřestavuje je ani samostatně
neověřuje jejich původní kompilační přepínače.
Samotný úspěšný build není potvrzením funkčnosti na fyzické desce.
Následující hardwarové výsledky dodal uživatel po testování prototypu.

## Hardwarová validace M2.1

Uživatel provedl test na skutečné desce rosco_m68k s CPU MC68010.
Prostředí: Windows, COM6, 38400 baud, 8N1, řízení toku None;
terminály Tera Term 5.6.1 a PuTTY.

| Ověřený scénář | Výsledek |
| --- | --- |
| Inicializace a vykreslení terminálové obrazovky | PASS |
| Běžné znaky a hexadecimální výpis přijatých bajtů | PASS |
| Rozpoznání šipek | PASS |
| Rozpoznání speciálních kláves | PASS; jednotlivý seznam kláves nebyl zaznamenán |
| Ctrl+A = `0x01` | PASS |
| Ctrl+B = `0x02` | PASS |
| Ctrl+C = `0x03` | PASS |
| Ctrl+P = `0x10` | PASS |
| Malé q = `0x71` | PASS |
| Velké Q = `0x51` | PASS |
| Odeslání binárního souboru s jediným bajtem `0x11` přes Tera Term ukončí program | PASS |
| Ctrl+Q v PuTTY ukončí program | PASS |

Po normálním ukončení se deska vrátila k očekávanému chování firmware/resetu.

**Známá anomálie Tera Term 5.6.1:** stisk Ctrl+Q odesílá nebo vede k přijetí
`0x1C` místo očekávaného `0x11`. Pozorování přetrvává s českým i anglickým
(US) rozložením klávesnice Windows a s řízením toku None. Ctrl+Q v PuTTY
funguje správně a samostatně odeslaný bajt `0x11` přes Tera Term také
vyvolá očekávané ukončení. Přesná příčina v Tera Termu nebo jeho konfiguraci
nebyla zjištěna; nelze ji připsat konkrétní komponentě. Prototyp ani libtermios
kvůli tomuto pozorování neobsahují žádnou obcházející úpravu.

**Stav:** základní funkčnost M2.1 je podle dodaných pozorování hardwarově
validována. Anomálie Tera Termu zůstává otevřená, ale neblokuje doporučení
k uzavření M2.1. Tento záznam nepotvrzuje všechny varianty speciálních kláves,
chybové cesty, přesnost timeoutů ani scénáře poškozených/neúplných ESC sekvencí.
Samostatná kontrola obnovy atributů a viditelnosti kurzoru nebyla v dodaných
výsledcích zaznamenána; implementace těchto kroků byla ověřena revizí zdroje.
