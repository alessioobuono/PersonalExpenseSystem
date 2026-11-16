// PersonalExpenseSystem - progetto universitario C++17 + SQLite3
// Compilazione Linux/macOS: g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp -lsqlite3 -o personal_expense_system

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

class SqlStatement {
public:
    SqlStatement(sqlite3* db, const std::string& sql) : db_(db), stmt_(nullptr) {
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }
    ~SqlStatement() { sqlite3_finalize(stmt_); }
    SqlStatement(const SqlStatement&) = delete;
    SqlStatement& operator=(const SqlStatement&) = delete;

    void bind(int position, const std::string& value) {
        if (sqlite3_bind_text(stmt_, position, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db_));
    }
    void bind(int position, std::int64_t value) {
        if (sqlite3_bind_int64(stmt_, position, value) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db_));
    }
    int step() {
        const int result = sqlite3_step(stmt_);
        if (result != SQLITE_ROW && result != SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db_));
        return result;
    }
    sqlite3_stmt* get() const { return stmt_; }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_;
};

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw std::runtime_error("Input terminato. Chiusura del programma.");
    }
    return trim(line);
}

// Conversione esatta in centesimi, senza floating point; accetta virgola o punto.
std::optional<std::int64_t> parseMoney(const std::string& input) {
    static const std::regex pattern(R"(^([0-9]+)(?:[.,]([0-9]{1,2}))?$)");
    std::smatch matches;
    if (!std::regex_match(input, matches, pattern)) return std::nullopt;
    std::int64_t euros = 0;
    constexpr std::int64_t max = std::numeric_limits<std::int64_t>::max();
    const std::string integerPart = matches[1];
    const std::string decimalPart = matches[2];
    const int centsPart = decimalPart.empty() ? 0 :
        (decimalPart.length() == 1 ? 10 * (decimalPart[0] - '0') :
                                    10 * (decimalPart[0] - '0') + decimalPart[1] - '0');
    for (const char c : integerPart) {
        const int digit = c - '0';
        const std::int64_t maxEuros = (max - centsPart) / 100;
        if (euros > (maxEuros - digit) / 10) return std::nullopt;
        euros = euros * 10 + digit;
    }
    if (euros > (max - centsPart) / 100) return std::nullopt;
    const std::int64_t amount = euros * 100 + centsPart;
    if (amount <= 0) return std::nullopt;
    return amount;
}

std::string formatMoney(std::int64_t cents) {
    std::ostringstream out;
    out << cents / 100 << ',' << std::setw(2) << std::setfill('0') << cents % 100;
    return out.str();
}

bool isValidMonth(const std::string& value) {
    static const std::regex re(R"(^[0-9]{4}-(0[1-9]|1[0-2])$)");
    return std::regex_match(value, re) && value.substr(0, 4) != "0000";
}

bool isValidDate(const std::string& value) {
    static const std::regex re(R"(^[0-9]{4}-(0[1-9]|1[0-2])-([0-2][0-9]|3[01])$)");
    if (!std::regex_match(value, re) || value.substr(0, 4) == "0000") return false;
    const int year = std::stoi(value.substr(0, 4));
    const int month = std::stoi(value.substr(5, 2));
    const int day = std::stoi(value.substr(8, 2));
    const bool leap = (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
    const int days[] = {0, 31, leap ? 29 : 28, 31, 30, 31, 30,
                        31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= days[month];
}

int menuChoice(const std::string& prompt) {
    std::cout << prompt;
    int choice = 0;
    if (!(std::cin >> choice)) {
        if (std::cin.eof()) throw std::runtime_error("Input terminato. Chiusura del programma.");
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return -1;
    }
    // Esclude input come "2abc" o "2 3".
    std::string remainder;
    std::getline(std::cin, remainder);
    return trim(remainder).empty() ? choice : -1;
}

std::string shorten(const std::string& value, std::size_t maxLength) {
    if (value.length() <= maxLength) return value;
    return value.substr(0, maxLength - 3) + "...";
}

class ExpenseSystem {
public:
    explicit ExpenseSystem(sqlite3* db) : db_(db) {}

    void manageCategories() {
        const std::string name = readLine("Nome della categoria: ");
        if (name.empty()) {
            std::cout << "Errore: il nome della categoria non puo' essere vuoto.\n";
            return;
        }
        SqlStatement existing(db_, "SELECT id FROM categories WHERE name = ? COLLATE NOCASE;");
        existing.bind(1, name);
        if (existing.step() == SQLITE_ROW) {
            std::cout << "La categoria esiste gia'.\n";
            return;
        }
        SqlStatement insert(db_, "INSERT INTO categories(name) VALUES(?);");
        insert.bind(1, name);
        insert.step();
        std::cout << "Categoria inserita correttamente.\n";
    }

    void insertExpense() {
        const std::string date = readLine("Data (YYYY-MM-DD): ");
        if (!isValidDate(date)) {
            std::cout << "Errore: data non valida (formato YYYY-MM-DD).\n";
            return;
        }
        const std::string input = readLine("Importo (es. 25,50): ");
        const auto amount = parseMoney(input);
        if (!amount) {
            std::cout << "Errore: l'importo deve essere maggiore di zero e avere max 2 decimali.\n";
            return;
        }
        const std::string category = readLine("Nome della categoria: ");
        const auto categoryId = findCategory(category);
        if (!categoryId) {
            std::cout << "Errore: la categoria non esiste.\n";
            return;
        }
        const std::string description = readLine("Descrizione (facoltativa): ");
        SqlStatement insert(db_, "INSERT INTO expenses(expense_date, amount_cents, category_id, description) "
                                 "VALUES (?, ?, ?, ?);");
        insert.bind(1, date);
        insert.bind(2, *amount);
        insert.bind(3, *categoryId);
        insert.bind(4, description);
        insert.step();
        std::cout << "Spesa inserita correttamente.\n";
    }

    void setBudget() {
        const std::string month = readLine("Mese (YYYY-MM): ");
        if (!isValidMonth(month)) {
            std::cout << "Errore: mese non valido (formato YYYY-MM).\n";
            return;
        }
        const std::string category = readLine("Nome della categoria: ");
        const auto categoryId = findCategory(category);
        if (!categoryId) {
            std::cout << "Errore: la categoria non esiste.\n";
            return;
        }
        const auto amount = parseMoney(readLine("Importo del budget: "));
        if (!amount) {
            std::cout << "Errore: il budget deve essere maggiore di zero e avere max 2 decimali.\n";
            return;
        }
        SqlStatement upsert(db_,
            "INSERT INTO budgets(budget_month, category_id, amount_cents) VALUES (?, ?, ?) "
            "ON CONFLICT(budget_month, category_id) DO UPDATE "
            "SET amount_cents = excluded.amount_cents;");
        upsert.bind(1, month);
        upsert.bind(2, *categoryId);
        upsert.bind(3, *amount);
        upsert.step();
        std::cout << "Budget mensile salvato correttamente.\n";
    }

    void reportByCategory() {
        SqlStatement rows(db_,
            "SELECT c.name, COALESCE(SUM(e.amount_cents), 0) "
            "FROM categories c LEFT JOIN expenses e ON e.category_id = c.id "
            "GROUP BY c.id, c.name ORDER BY c.name COLLATE NOCASE;");
        std::cout << "\nTOTALE SPESE PER CATEGORIA\n";
        std::cout << std::left << std::setw(24) << "Categoria" << "Totale speso (EUR)\n";
        std::cout << std::string(46, '-') << '\n';
        bool found = false;
        while (rows.step() == SQLITE_ROW) {
            found = true;
            const char* value = reinterpret_cast<const char*>(sqlite3_column_text(rows.get(), 0));
            std::cout << std::left << std::setw(24) << shorten(value ? value : "", 23)
                      << formatMoney(sqlite3_column_int64(rows.get(), 1)) << '\n';
        }
        if (!found) std::cout << "Nessuna categoria presente.\n";
    }

    void reportBudget() {
        // UNION include sia mesi con budget sia mesi con spese prive di budget.
        SqlStatement rows(db_, R"SQL(
            WITH spent AS (
                SELECT substr(expense_date, 1, 7) AS month, category_id,
                       SUM(amount_cents) AS spent_cents
                FROM expenses
                GROUP BY substr(expense_date, 1, 7), category_id
            ),
            pairs AS (
                SELECT month, category_id FROM spent
                UNION
                SELECT budget_month AS month, category_id FROM budgets
            )
            SELECT p.month, c.name, b.amount_cents,
                   COALESCE(s.spent_cents, 0) AS spent_cents
            FROM pairs p
            JOIN categories c ON c.id = p.category_id
            LEFT JOIN budgets b ON b.budget_month = p.month
                               AND b.category_id = p.category_id
            LEFT JOIN spent s ON s.month = p.month
                             AND s.category_id = p.category_id
            ORDER BY p.month, c.name COLLATE NOCASE;
        )SQL");
        std::cout << "\nSPESE MENSILI VS BUDGET\n";
        bool found = false;
        while (rows.step() == SQLITE_ROW) {
            found = true;
            const std::string month = reinterpret_cast<const char*>(sqlite3_column_text(rows.get(), 0));
            const std::string category = reinterpret_cast<const char*>(sqlite3_column_text(rows.get(), 1));
            const std::int64_t spent = sqlite3_column_int64(rows.get(), 3);
            std::cout << "\nMese: " << month << "\nCategoria: " << category << '\n';
            if (sqlite3_column_type(rows.get(), 2) == SQLITE_NULL) {
                std::cout << "Budget: non definito\nSpeso: " << formatMoney(spent)
                          << " EUR\nStato: BUDGET NON DEFINITO\n";
            } else {
                const std::int64_t budget = sqlite3_column_int64(rows.get(), 2);
                std::cout << "Budget: " << formatMoney(budget)
                          << " EUR\nSpeso: " << formatMoney(spent) << " EUR\n";
                if (spent > budget) {
                    std::cout << "Stato: SUPERAMENTO BUDGET\n";
                } else if (spent == budget) {
                    std::cout << "Stato: BUDGET RAGGIUNTO\n";
                } else {
                    std::cout << "Stato: NEL BUDGET\n";
                }
            }
        }
        if (!found) std::cout << "Nessuna spesa o budget presente.\n";
    }

    void reportAllExpenses() {
        SqlStatement rows(db_,
            "SELECT e.expense_date, c.name, e.amount_cents, e.description "
            "FROM expenses e JOIN categories c ON e.category_id = c.id "
            "ORDER BY e.expense_date ASC, e.id ASC;");
        std::cout << "\nELENCO COMPLETO DELLE SPESE (ORDINE CRONOLOGICO)\n";
        std::cout << std::left << std::setw(13) << "Data" << std::setw(22) << "Categoria"
                  << std::setw(14) << "Importo (EUR)" << "Descrizione\n";
        std::cout << std::string(73, '-') << '\n';
        bool found = false;
        while (rows.step() == SQLITE_ROW) {
            found = true;
            const auto getText = [&](int i) {
                const char* s = reinterpret_cast<const char*>(sqlite3_column_text(rows.get(), i));
                return std::string(s ? s : "");
            };
            std::cout << std::left << std::setw(13) << getText(0)
                      << std::setw(22) << shorten(getText(1), 21)
                      << std::setw(14) << formatMoney(sqlite3_column_int64(rows.get(), 2))
                      << shorten(getText(3), 50) << '\n';
        }
        if (!found) std::cout << "Nessuna spesa presente.\n";
    }

    void reportsMenu() {
        bool back = false;
        while (!back) {
            std::cout << "\n---------- MENU REPORT ----------\n"
                      << "1. Totale spese per categoria\n"
                      << "2. Spese mensili vs budget\n"
                      << "3. Elenco completo delle spese ordinate per data\n"
                      << "4. Ritorna al menu principale\n"
                      << "---------------------------------\n";
            const int choice = menuChoice("Inserisci la tua scelta: ");
            try {
                switch (choice) {
                    case 1: reportByCategory(); break;
                    case 2: reportBudget(); break;
                    case 3: reportAllExpenses(); break;
                    case 4: back = true; break;
                    default: std::cout << "Scelta non valida. Riprovare.\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Errore durante il report: " << e.what() << '\n';
            }
        }
    }

    void run() {
        std::cout << "Benvenuto nel Sistema di Gestione delle Spese Personali!\n";
        bool running = true;
        while (running) {
            std::cout << "\n-------------------------\n"
                      << " SISTEMA SPESE PERSONALI\n"
                      << "-------------------------\n"
                      << "1. Gestione Categorie\n"
                      << "2. Inserisci Spesa\n"
                      << "3. Definisci Budget Mensile\n"
                      << "4. Visualizza Report\n"
                      << "5. Esci\n"
                      << "-------------------------\n";
            const int choice = menuChoice("Inserisci la tua scelta: ");
            try {
                switch (choice) {
                    case 1: manageCategories(); break;
                    case 2: insertExpense(); break;
                    case 3: setBudget(); break;
                    case 4: reportsMenu(); break;
                    case 5: running = false; break;
                    default: std::cout << "Scelta non valida. Riprovare.\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Errore durante l'operazione: " << e.what() << '\n';
                if (!std::cin) throw;
            }
        }
        std::cout << "Arrivederci!\n";
    }

private:
    std::optional<std::int64_t> findCategory(const std::string& name) {
        SqlStatement find(db_, "SELECT id FROM categories WHERE name = ? COLLATE NOCASE;");
        find.bind(1, name);
        if (find.step() == SQLITE_ROW) return sqlite3_column_int64(find.get(), 0);
        return std::nullopt;
    }
    sqlite3* db_;
};

std::string readWholeFile(const fs::path& filename) {
    std::ifstream input(filename);
    if (!input) throw std::runtime_error("Impossibile leggere lo script SQL: " + filename.string());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

int main() {
    sqlite3* db = nullptr;
    try {
        const fs::path root = fs::current_path();
        const fs::path dataFolder = root / "data";
        const fs::path database = dataFolder / "expenses.db";
        const bool needsInit = !fs::exists(database);
        if (needsInit && !fs::exists(root / "sql" / "database.sql")) {
            throw std::runtime_error("Esegui il programma dalla cartella PersonalExpenseSystem: manca sql/database.sql.");
        }
        fs::create_directories(dataFolder);
        if (sqlite3_open(database.string().c_str(), &db) != SQLITE_OK)
            throw std::runtime_error(db ? sqlite3_errmsg(db) : "Errore apertura database SQLite.");
        char* sqliteError = nullptr;
        if (sqlite3_exec(db, "PRAGMA foreign_keys = ON; PRAGMA busy_timeout = 3000;", nullptr, nullptr, &sqliteError) != SQLITE_OK) {
            const std::string message = sqliteError ? sqliteError : "Errore SQLite";
            sqlite3_free(sqliteError);
            throw std::runtime_error(message);
        }
        if (needsInit) {
            const std::string sql = readWholeFile(root / "sql" / "database.sql");
            if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &sqliteError) != SQLITE_OK) {
                const std::string message = sqliteError ? sqliteError : "Errore inizializzazione SQLite";
                sqlite3_free(sqliteError);
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                sqlite3_close(db);
                db = nullptr;
                fs::remove(database);
                throw std::runtime_error("Inizializzazione non riuscita: " + message);
            }
            std::cout << "Database inizializzato con i dati di esempio.\n";
        }
        ExpenseSystem system(db);
        system.run();
        sqlite3_close(db);
        return 0;
    } catch (const std::exception& e) {
        if (db) sqlite3_close(db);
        std::cerr << "Errore: " << e.what() << '\n';
        return 1;
    }
}
