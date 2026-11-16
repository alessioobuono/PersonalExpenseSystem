-- PersonalExpenseSystem - SQLite 3
-- Eseguito automaticamente una sola volta, alla prima apertura del programma.
-- Gli importi sono memorizzati in centesimi, per evitare errori di arrotondamento.
PRAGMA foreign_keys = ON;

BEGIN TRANSACTION;

CREATE TABLE categories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL COLLATE NOCASE,
    CONSTRAINT uq_category_name UNIQUE (name),
    CONSTRAINT ck_category_name CHECK (length(trim(name)) > 0)
);

CREATE TABLE expenses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    expense_date TEXT NOT NULL,
    amount_cents INTEGER NOT NULL,
    category_id INTEGER NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    CONSTRAINT ck_expense_date CHECK (
        expense_date GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
        AND substr(expense_date, 6, 2) BETWEEN '01' AND '12'
        AND substr(expense_date, 9, 2) BETWEEN '01' AND '31'
    ),
    CONSTRAINT ck_expense_amount CHECK (amount_cents > 0),
    CONSTRAINT fk_expense_category FOREIGN KEY (category_id)
        REFERENCES categories(id) ON UPDATE CASCADE ON DELETE RESTRICT
);

CREATE TABLE budgets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    budget_month TEXT NOT NULL,
    category_id INTEGER NOT NULL,
    amount_cents INTEGER NOT NULL,
    CONSTRAINT ck_budget_month CHECK (
        budget_month GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]'
        AND substr(budget_month, 6, 2) BETWEEN '01' AND '12'
    ),
    CONSTRAINT ck_budget_amount CHECK (amount_cents > 0),
    CONSTRAINT uq_budget_month_category UNIQUE (budget_month, category_id),
    CONSTRAINT fk_budget_category FOREIGN KEY (category_id)
        REFERENCES categories(id) ON UPDATE CASCADE ON DELETE RESTRICT
);

CREATE INDEX idx_expenses_date ON expenses(expense_date);
CREATE INDEX idx_expenses_category_date ON expenses(category_id, expense_date);

-- Categorie dimostrative.
INSERT INTO categories (name) VALUES ('Alimentari');
INSERT INTO categories (name) VALUES ('Trasporti');
INSERT INTO categories (name) VALUES ('Tempo libero');

-- Gennaio 2025: Alimentari = 320,50; Trasporti = 120,00.
INSERT INTO expenses (expense_date, amount_cents, category_id, description)
VALUES ('2025-01-05', 9000, (SELECT id FROM categories WHERE name = 'Alimentari'), 'Spesa supermercato');
INSERT INTO expenses (expense_date, amount_cents, category_id, description)
VALUES ('2025-01-15', 2550, (SELECT id FROM categories WHERE name = 'Alimentari'), 'Pranzo');
INSERT INTO expenses (expense_date, amount_cents, category_id, description)
VALUES ('2025-01-28', 20500, (SELECT id FROM categories WHERE name = 'Alimentari'), 'Spesa mensile');
INSERT INTO expenses (expense_date, amount_cents, category_id, description)
VALUES ('2025-01-12', 7000, (SELECT id FROM categories WHERE name = 'Trasporti'), 'Carburante');
INSERT INTO expenses (expense_date, amount_cents, category_id, description)
VALUES ('2025-01-25', 5000, (SELECT id FROM categories WHERE name = 'Trasporti'), 'Treno');
INSERT INTO expenses (expense_date, amount_cents, category_id, description)
VALUES ('2025-02-10', 3990, (SELECT id FROM categories WHERE name = 'Tempo libero'), 'Cinema e cena');

-- Budget dimostrativi: uno superato, uno rispettato, uno senza spese.
INSERT INTO budgets (budget_month, category_id, amount_cents)
VALUES ('2025-01', (SELECT id FROM categories WHERE name = 'Alimentari'), 30000);
INSERT INTO budgets (budget_month, category_id, amount_cents)
VALUES ('2025-01', (SELECT id FROM categories WHERE name = 'Trasporti'), 15000);
INSERT INTO budgets (budget_month, category_id, amount_cents)
VALUES ('2025-02', (SELECT id FROM categories WHERE name = 'Alimentari'), 20000);

COMMIT;
