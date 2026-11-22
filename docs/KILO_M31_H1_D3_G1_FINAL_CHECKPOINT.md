# Kilo68k M3.1 — závěrečný checkpoint H1-D3-G1

## Výsledek milníku

| Ověření | Výsledek a rozsah |
| --- | --- |
| H1-D3-H1 na fyzickém MC68010 | **PASS.** Operátor potvrdil úplný smoke test čtení z SD/FAT a návrat do firmware warm boot. Průběh, nastavení a přepis jsou v [hardwarovém checkpointu](KILO_M31_H1_D3_HARDWARE_PASS_CHECKPOINT.md). |
| Přenositelný build profilu D3 | **PASS.** Dříve ověřený výsledek H1-D3-G1. Požadavky na nástroje, vstupy a výstupní adresář popisuje [profil D3](../code/software/newlib/m31a/b3_v2fix/README.md). |
| Izolovaný build z commitnutého stromu | **PASS.** Dříve ověřený výsledek H1-D3-G1 pro integrační commit níže. Poznámka o čekání na tento krok v README profilu zachycuje stav před integrací. |
| Strojový audit | **PASS.** Automatické kontroly binárky, provenience a omezení na čtení. |
| Negativní audity | **14/14 PASS.** Historicky ověřené odmítnutí změněných vstupů ze zamýšlených důvodů. |
| Úplný ruční audit | **CONDITIONAL.** K uzavření je nutný doložený ruční přezkum vztahující se ke konkrétní dvojici ELF/BIN. |

Výsledky buildů a auditů jsou historické. Při kontrole po restartu ani při
zápisu tohoto checkpointu se buildy, audity a testy znovu nespouštěly.
Rozsah auditů a validační evidence jsou v [hardwarovém checkpointu, §7 a §12](KILO_M31_H1_D3_HARDWARE_PASS_CHECKPOINT.md#7-h1-d3-i1-host-and-target-validation)
a v [profilu D3](../code/software/newlib/m31a/b3_v2fix/README.md#audit-interpretation--limits).
Dřívější stav je zachován v [checkpointu původního H1 selhání](KILO_M31_B3_T2_H1_FAILURE_RESUME_CHECKPOINT.md)
a [checkpointu integrace B3](KILO_M31_B3_I1_FINAL_CHECKPOINT.md).

### Identita hardwarově ověřeného obrazu

- Soubor `m31-csd-v2fix-diag.bin`, 62 044 bajtů.
- SHA-256 `d0110563cf819b6023d3b5b31883c7eb0aa222640cbdfc39978e5af233f10c0f`.
- CRC-32 `0xEBB863C5`. Loader na desce potvrdil velikost a CRC-32 souboru
  zavedeného z `/CSDV2FIX.BIN`. SHA-256 pochází z lokálního referenčního souboru.
- Záznam fyzického testu se vztahuje k tomuto historickému obrazu. Nový build
  evidujte samostatně s jeho vlastními otisky.

## Integrace a kontrola po restartu

Integrační commit `636b6466bb6354e2594660c56305c1a0afef6994` zahrnul
127 schválených souborů. Push větve `feature/kilo-editor` do forku
`PavelKCZ/rosco_m68k` byl úspěšný a ověřený na GitHubu před restartem.
Při kontrole po restartu ukazoval lokální `origin/feature/kilo-editor`
na stejný commit a reflog obsahoval záznam `update by push`.

Kontrola po restartu proběhla pouze čtením. Potvrdila očekávaný HEAD,
prázdný staging a zachování všech 7 záměrně upravených sledovaných souborů
i všech 80 nesledovaných souborů. Předintegrační inventář obsahoval 2 429
souborů. Všechny byly dohledány a 2 426 mělo totožné otisky. Zbývající tři
soubory M3.1 odpovídaly obsahu integračního commitu. `git fsck` ani
`git diff --check` nehlásily chyby. Kontrola nenašla Git zámky ani známky
rozpracované operace.

Trvalé archivy v `/home/pavelk/rosco/`:

- `KILO68K_M31_H1_D3_HARDWARE_PASS_2026-11-21/` — všech 1 745 položek
  hlavního `SHA256SUMS` prošlo, stejně jako oba vnořené manifesty evidence
  H1-D3-I1. Výchozí inventář je v `metadata/repository-baseline.json`.
- `KILO68K_M31_HISTORICAL_RUNTIME_FIXTURE_2026-11-21/` — obě položky
  `SHA256SUMS` prošly. Historický `runtime-smoke.bin` má 44 980 bajtů a
  SHA-256 `066e667d849623ac682c8153542af1f688209f28c9f09a0bedaafba6943ebeb5`.
  Je určen pro regresi runtime.

Původní dočasné adresáře v `/tmp` po restartu chyběly. Starší zdrojové
manifesty zachovávají původní otisky a vykazují stejné neshody v archivu
i v pracovním stromu. Aktuální otisky profilu určuje jeho `SHA256SUMS`
a `PROVENANCE.json`.

## Otevřené otázky

- Identita přesné aktivní ROM. Nápisy bootu a SD menu zachycují verze hlášené
  firmwarem.
- Úplná horní mez spotřeby zásobníku včetně firmware, IRQ a knihoven.
- Úplný ruční přezkum instrukcí pro MC68010 včetně přilinkovaných knihoven.
- Kompatibilita s širším výběrem SD karet a FAT svazků, nezávislé ověření
  fyzického CSD CRC a pokrytí zátěžových a poruchových scénářů.

## Pokračování v Codexu nebo Claude Code

1. Otevřete repozitář `/home/pavelk/rosco/rosco_m68k` na větvi
   `feature/kilo-editor`. Přečtěte [AGENTS.md](../AGENTS.md),
   [F3 checkpoint](KILO_F3_CHECKPOINT.md),
   [kontext portu](KILO_PORTING_CONTEXT.md), tento dokument a profil D3.
2. Ověřte větev, HEAD, staging a pracovní strom pomocí příkazů pouze pro
   čtení. Pro `git status` použijte `GIT_OPTIONAL_LOCKS=0`. V historii
   vyhledejte integrační commit a dokumentační commit tohoto souboru.
   Zachovejte záměrné úpravy, nesledovanou evidenci i oba archivy.
3. Při ověřování historie použijte trvalé archivy a jejich manifesty.
   Při nesouladu zastavte a popište rozdíl. Nové buildy, testy, opravy,
   přístup k hardwaru nebo COM6 a změny v Gitu vyžadují výslovné zadání.
