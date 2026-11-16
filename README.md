# PersonalExpenseSystem

**Progetto finale di programmazione - AREA L9**  
Sistema di gestione delle spese personali e del budget, da console, in **C++17** e **SQLite 3**. L'applicazione è destinata a un solo utente e non richiede accesso a Internet.

## Struttura del repository

```text
PersonalExpenseSystem/
├── src/
│   └── main.cpp
├── sql/
│   └── database.sql
├── demo/
│   └── demo_video.mp4
└── README.md
```

La cartella `data/` con il database `expenses.db` è **creata automaticamente al primo avvio**; è un file locale di lavoro e non va caricato nel repository. Il file eseguibile generato dalla compilazione non va caricato nel repository.

## Requisiti

- Compilatore C++ con supporto a **C++17** e `std::filesystem` (GCC 9+, Clang recente o equivalente).
- **SQLite 3**, inclusi header `sqlite3.h` e libreria di sviluppo per il collegamento.
- Librerie standard C++ usate: `iostream`, `string`, `iomanip`, `regex`, `filesystem`, `fstream`, `optional`, `sstream` e altre indicate nel sorgente.
- Nessun framework web, GUI o servizio esterno.

## Installazione e compilazione

Aprire il terminale **nella cartella radice `PersonalExpenseSystem`** (quella che contiene `src` e `sql`).

### Linux (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install g++ libsqlite3-dev
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp -lsqlite3 -o personal_expense_system
./personal_expense_system
```

### Windows (MSYS2 UCRT64)

Installare MSYS2 e aprire **MSYS2 UCRT64**. Installare il compilatore e la libreria SQLite:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-sqlite3
```

Dalla cartella del progetto:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp -lsqlite3 -o PersonalExpenseSystem.exe
./PersonalExpenseSystem.exe
```

Per Windows è possibile usare anche Visual Studio con SQLite 3 correttamente collegato, ma i comandi sopra sono quelli di riferimento.

### macOS (Homebrew)

```bash
brew install sqlite3
clang++ -std=c++17 src/main.cpp -I"$(brew --prefix sqlite3)/include" -L"$(brew --prefix sqlite3)/lib" -lsqlite3 -o personal_expense_system
./personal_expense_system
```

## Primo avvio e dati di esempio

Al primo avvio il programma crea `data/expenses.db` ed esegue **integralmente** `sql/database.sql`, che contiene `CREATE TABLE`, indici e `INSERT` dimostrativi. Negli avvii successivi riapre il database esistente, preservando le modifiche.

Per ricominciare dai dati di esempio, **chiudere il programma**, eliminare solo `data/expenses.db` e riavviarlo. Non eliminare il database se contiene dati che si vogliono conservare.

Il sistema include le categorie `Alimentari`, `Trasporti` e `Tempo libero`, alcuni movimenti a gennaio/febbraio 2025 e budget di esempio. In particolare, il report mostra il superamento del budget Alimentari a gennaio (320,50 EUR spesi su 300,00 EUR di budget).

## Funzionalità e istruzioni d'uso

**Menu principale** (ripetuto fino alla scelta `5`):

1. **Gestione Categorie:** digitare il nome; il programma rifiuta nomi vuoti e duplicati senza distinzione tra maiuscole e minuscole.
2. **Inserisci Spesa:** digitare una data reale nel formato `YYYY-MM-DD`, un importo positivo (es. `25,50` oppure `25.50`), una categoria già esistente e una descrizione facoltativa.
3. **Definisci Budget Mensile:** digitare un mese `YYYY-MM`, una categoria esistente e un importo positivo. Se il budget per la stessa categoria/mese esiste, viene aggiornato.
4. **Visualizza Report:** aprire il sottomenu per totale per categoria, spese mensili confrontate con il budget ed elenco completo ordinato per data; l'opzione `4` del sottomenu torna al menu principale.
5. **Esci:** termina il programma.

Gli importi sono archiviati come **centesimi interi**, evitando imprecisioni tipiche del formato `float`. Non è consentito un importo zero, negativo, non numerico o con più di due decimali.

Il report "Spese mensili vs budget" mostra anche categorie/mese che hanno spese **senza budget definito** e budget **senza spese**. Gli stati possibili sono `SUPERAMENTO BUDGET`, `BUDGET RAGGIUNTO`, `NEL BUDGET` e `BUDGET NON DEFINITO`.

## Schema del database

- `categories(id PK, name UNIQUE NOT NULL)`: catalogo categorie.
- `expenses(id PK, expense_date NOT NULL, amount_cents NOT NULL CHECK > 0, category_id FK NOT NULL, description NOT NULL)`: movimenti.
- `budgets(id PK, budget_month NOT NULL, category_id FK NOT NULL, amount_cents NOT NULL CHECK > 0, UNIQUE(budget_month, category_id))`: limiti mensili.

Relazioni: `categories (1) -> (N) expenses` e `categories (1) -> (N) budgets`.

I vincoli sono definiti esplicitamente nel file SQL: `PRIMARY KEY`, `FOREIGN KEY`, `CHECK`, `UNIQUE`, `NOT NULL`. Le chiavi esterne sono abilitate tramite `PRAGMA foreign_keys = ON`. Il programma usa query preparate e parametri associati, per evitare concatenazioni pericolose dell'input nelle query SQL.

Il controllo SQL sulle date verifica il formato e i principali intervalli; la **validazione completa del calendario** (per esempio 29 febbraio negli anni bisestili) è effettuata in C++.

## Interrogazioni principali

- Totale per categoria: `LEFT JOIN`, `SUM`, `GROUP BY`.
- Spese mensili vs budget: `WITH` (CTE), `SUM`, `GROUP BY`, `UNION`, `LEFT JOIN`.
- Elenco cronologico: `JOIN`, `ORDER BY`.
- Aggiornamento budget: `INSERT ... ON CONFLICT ... DO UPDATE`.

## Demo video

Il file `demo/demo_video.mp4` riproduce una sessione dimostrativa del programma compilato: avvio, menu, inserimento di categoria e spesa, modifica budget, report e uscita. Il testo mostrato deriva dall'esecuzione reale del programma su un ambiente di prova; è presentato in formato video per documentare i passaggi.

## Repository e consegna

La **consegna ufficiale richiesta dal docente è un singolo PDF** che riporti integralmente lo script SQL, lo schema logico e il **link completo al repository GitHub personale**. Questo README fa parte del repository, non sostituisce il PDF.

Prima della consegna:

1. Creare un repository GitHub, per esempio `PersonalExpenseSystem`.
2. Caricare **solo** `src/main.cpp`, `sql/database.sql`, `demo/demo_video.mp4`, `README.md` (non il database generato o l'eseguibile).
3. Inserire il proprio URL reale `https://github.com/<username>/PersonalExpenseSystem` nel PDF finale, sostituendo il segnaposto.
4. Verificare che il repository sia accessibile al docente e che i file siano presenti.

## Licenza e uso

Elaborato didattico dimostrativo. Non contiene funzioni di gestione multiutente, sincronizzazione bancaria o cifratura dei dati.
