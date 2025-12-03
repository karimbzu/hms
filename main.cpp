#include "crow.h"     // including crow frameword
#include <sqlite3.h>
#include <iostream>
#include <sstream>

int main()
{
    crow::SimpleApp app;
      bool isLoggedIn = false;
    // Initialize SQLite database
    sqlite3 *db;
    if (sqlite3_open("hms.db", &db))
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    // Create table if not exists
    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "phone TEXT NOT NULL, "
        "disease TEXT NOT NULL, "
        "date TEXT NOT NULL);";

    const char *create_accounts_table =
       "CREATE TABLE IF NOT EXISTS accounts ("
       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
       "username TEXT NOT NULL UNIQUE, "
       "password TEXT NOT NULL);";


      char *errMsg = nullptr;
      if (sqlite3_exec(db, create_accounts_table, nullptr, nullptr, &errMsg) != SQLITE_OK)
      {
          std::cerr << "SQL error: " << errMsg << std::endl;
         sqlite3_free(errMsg);
      }

      // Insert default admin only if table empty
      sqlite3_stmt *checkStmt;
      sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM accounts;", -1, &checkStmt, nullptr);
      sqlite3_step(checkStmt);
      int count = sqlite3_column_int(checkStmt, 0);
     sqlite3_finalize(checkStmt);

     if (count == 0)
    {
       sqlite3_exec(db, "INSERT INTO accounts (username, password) VALUES ('admin', '1234');", nullptr, nullptr, nullptr);
    }


    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
       std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
// Public landing page (accessible by everyone)
CROW_ROUTE(app, "/")([]() {
    std::ostringstream page;
    page << R"(
    <html>
    <head>
        <title>Welcome — Hospital Management (HMS) </title>
        <style>
            body { font-family: Arial, sans-serif; background: #f7fbff; display:flex; align-items:center; justify-content:center; height:100vh; }
            .box { width:420px; background:white; padding:24px; border-radius:8px; box-shadow:0 6px 20px rgba(0,0,0,0.08); text-align:center; }
            a { display:inline-block; margin:10px; padding:10px 16px; border-radius:6px; text-decoration:none; border:1px solid #007bff; color:#007bff; }
            a.primary { background:#007bff; color:white; border:none; }
        </style>
    </head>
    <body>
        <div class='box'>
            <h1>Welcome to HMS</h1>
            <p>Manage patient appointments and records quickly.</p>
            <div>
                <a href="/login" class="primary">Login</a>
                <a href="/about">About</a>
            </div>
        </div>
    </body>
    </html>
    )";
    return crow::response(page.str());
});

CROW_ROUTE(app, "/login")([]() {
    std::ostringstream page;
    page << R"(
    <html>
    <head>
        <title>Login Page</title>
        <style>
            body {
                font-family: Arial;
                background: #e9f0ff;
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
            }
            .box {
                width: 350px;
                background: white;
                padding: 25px;
                border-radius: 10px;
                box-shadow: 0 4px 10px rgba(0,0,0,0.2);
            }
            input, button {
                width: 100%;
                padding: 10px;
                margin: 8px 0;
                border-radius: 5px;
                border: 1px solid #aaa;
            }
            button {
                background: #007bff;
                color: white;
                border: none;
                cursor: pointer;
            }
        </style>
    </head>
    <body>
        <div class='box'>
            <h2 style='text-align:center;'>Login</h2>
            <input id='username' placeholder='Enter username'>
            <input id='password' type='password' placeholder='Enter password'>
            <button onclick='login()'>Login</button>
        </div>

        <script>
            async function login() {
                let username = document.getElementById("username").value;
                let password = document.getElementById("password").value;

                let res = await fetch("/auth", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ username, password })
                });

                if (res.status === 200) {
                    window.location.href = "/dashboard";
                } else {
                    alert("Incorrect Username or Password!");
                }
            }
        </script>
    </body>
    </html>
    )";
    return crow::response(page.str());
});
CROW_ROUTE(app, "/logout")([&](){
    isLoggedIn = false;
    return crow::response(302, "<script>window.location='/login';</script>");
});

CROW_ROUTE(app, "/about")([]() {
    std::ostringstream page;
    page << R"(
    <html>
    <head>
        <title>About Us - HMS</title>
        <style>
            body {
                margin: 0;
                font-family: 'Segoe UI', sans-serif;
                background: linear-gradient(135deg, #6dd5fa, #ffffff);
            }
            .container {
                width: 85%;
                margin: 50px auto;
                background: white;
                padding: 40px;
                border-radius: 16px;
                box-shadow: 0 12px 25px rgba(0,0,0,0.15);
            }
            h1 {
                text-align: center;
                color: #007bff;
                margin-bottom: 10px;
            }
            .subtitle {
                text-align: center;
                color: #555;
                margin-bottom: 30px;
                font-size: 18px;
            }
            .section {
                margin-top: 30px;
            }
            .section h2 {
                color: #333;
                border-left: 5px solid #007bff;
                padding-left: 10px;
            }
            .section p {
                color: #555;
                line-height: 1.8;
                font-size: 16px;
            }
            .cards {
                display: flex;
                gap: 20px;
                margin-top: 25px;
                flex-wrap: wrap;
            }
            .card {
                flex: 1;
                background: #f5f9ff;
                padding: 20px;
                border-radius: 12px;
                box-shadow: 0 4px 10px rgba(0,0,0,0.1);
                min-width: 220px;
            }
            .card h3 {
                color: #007bff;
                margin-bottom: 10px;
            }
            .back-btn {
                margin-top: 40px;
                display: inline-block;
                padding: 12px 20px;
                background: #007bff;
                color: white;
                border-radius: 8px;
                text-decoration: none;
                transition: 0.3s;
            }
            .back-btn:hover {
                background: #0056b3;
            }
            footer {
                text-align: center;
                margin-top: 40px;
                color: #777;
                font-size: 14px;
            }
        </style>
    </head>

    <body>
        <div class="container">
            <h1>About Our Hospital Management System</h1>
            <div class="subtitle">Smart Healthcare | Simple Management | Secure Records</div>

            <div class="section">
                <h2>Our Mission</h2>
                <p>
                    Our mission is to simplify hospital operations through a modern, secure,
                    and user-friendly Hospital Management System that improves efficiency,
                    reduces paperwork, and ensures better patient care.
                </p>
            </div>

            <div class="section">
                <h2>What We Do</h2>
                <p>
                    Our HMS helps hospitals manage patient records, appointments, and medical
                    data digitally with accuracy and speed. It allows staff to focus more on
                    patient care rather than manual documentation.
                </p>
            </div>

            <div class="section">
                <h2>Why Choose Us?</h2>
                <div class="cards">
                    <div class="card">
                        <h3>✅ Secure Data</h3>
                        <p>All patient data is safely stored with proper authentication.</p>
                    </div>
                    <div class="card">
                        <h3>⚡ Fast Access</h3>
                        <p>Quick access to appointments and records in real time.</p>
                    </div>
                    <div class="card">
                        <h3>💻 User Friendly</h3>
                        <p>Simple interface that anyone can use without training.</p>
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Our Vision</h2>
                <p>
                    We aim to digitize healthcare services and make hospital operations smarter,
                    faster, and more reliable for both patients and healthcare professionals.
                </p>
            </div>

            <center>
                <a href="/" class="back-btn">← Back to Home</a>
            </center>

            <footer>
                © 2025 Hospital Management System | Developed for Academic Project
            </footer>
        </div>
    </body>
    </html>
    )";
    return crow::response(page.str());
});


CROW_ROUTE(app, "/auth").methods("POST"_method)([&](const crow::request& req){
    auto data = crow::json::load(req.body);
    if (!data || !data.has("username") || !data.has("password"))
        return crow::response(400, "Invalid");

    std::string username = data["username"].s();
    std::string password = data["password"].s();

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT * FROM accounts WHERE username=? AND password=?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) ok = true;

    sqlite3_finalize(stmt);

    if (ok) {
        isLoggedIn = true;
        return crow::response(200, "Login OK");
    }

    return crow::response(401, "Invalid credentials");
});
//Home page
CROW_ROUTE(app, "/dashboard")([&]() {
    if (!isLoggedIn)
        return crow::response(302, "<script>window.location='/login';</script>");
        std::ostringstream page;
        page << R"(
            <html>
            <head>
                              <title>Home Page</title>
             <style>
                    body {
                        font-family: Arial, sans-serif;
                        background:url('hms.png');
                        background-size: cover;
                        margin: 0;
                        padding: 0;
                    }
                    .container {
                        width: 80%;
                        margin: 40px auto;
                        background: #fff;
                        border-radius: 10px;
                        padding: 20px 40px;
                        box-shadow: 0 4px 10px rgba(0,0,0,0.1);
                    }
                    h2, h3 {
                        text-align: center;
                        color: #333;
                    }
                    input, button {
                        padding: 10px;
                        margin: 5px;
                        border-radius: 5px;
                        border: 1px solid #ccc;
                        font-size: 14px;
                    }
                    button {
                        cursor: pointer;
                        background-color: #007bff;
                        color: white;
                        border: none;
                        transition: 0.2s;
                    }
                    button:hover {
                        background-color: #0056b3;
                    }
                    table {
                        width: 100%;
                        border-collapse: collapse;
                        margin-top: 20px;
                    }
                    th, td {
                        padding: 10px;
                        border: 1px solid #ddd;
                        text-align: center;
                    }
                    th {
                        background-color: #007bff;
                        color: white;
                    }
                    tr:nth-child(even) {
                        background-color: #f9f9f9;
                    }
                    .action-btn {
                        padding: 6px 12px;
                        font-size: 13px;
                        border-radius: 4px;
                        margin: 2px;
                    }
                    .edit-btn {
                        background-color: #28a745;
                        color: white;
                        border: none;
                    }
                    .edit-btn:hover {
                        background-color: #1e7e34;
                    }
                    .delete-btn {
                        background-color: #dc3545;
                        color: white;
                        border: none;
                    }
                    .delete-btn:hover {
                        background-color: #b02a37;
                    }
                    .top-right-icon {
                       position: absolute;
                       top: 15px;
                       right: 20px;
                    }
                    .top-right-icon img {
                            width: 40px;
                            height: 40px;
                            cursor: pointer;
                    }
                </style>
                <script>
                    async function addUser() {
                        const name = document.getElementById("name").value;
                        const phone = document.getElementById("phone").value;
                        const disease = document.getElementById("disease").value;
                        const date = document.getElementById("date").value;
                        if (!name || !phone || !disease || !date) {
                            alert("Name, Phone number, Disease and Appointment Date are required!");
                            return;
                        }
                        await fetch("/add", {
                            method: "POST",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ name, phone, disease, date })
                        });
                        document.getElementById("name").value = "";
                        document.getElementById("phone").value = "";
                        document.getElementById("disease").value = "";
                        document.getElementById("date").value = "";
                        loadUsers();
                    }

                    async function editUser(id) {
                        const name = prompt("Enter new name:");
                        const phone = prompt("Enter new phone number:");
                        const disease = prompt("Enter new disease:");
                        const date = prompt("Enter new Appointment date");
                        if (!name || !phone || !disease || !date) {
                            alert("All fields are required!");
                            return;
                        }
                        await fetch("/edit", {
                            method: "POST",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ id, name, phone, disease, date })
                        });
                        loadUsers();
                    }

                    async function deleteUser(id) {
                        if (!confirm("Are you sure you want to delete this user?")) return;
                        await fetch("/delete", {
                            method: "POST",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ id })
                        });
                        loadUsers();
                    }

                    async function loadUsers() {
                        const res = await fetch("/users");
                        const users = await res.json();
                        let html = "<table><tr><th>ID</th><th>Name</th><th>Phone Number</th><th>Disease</th><th>Appointment Date</th><th>Actions</th></tr>";
                        for (const u of users) {
                            html += `<tr>
                                        <td>${u.id}</td>
                                        <td>${u.name}</td>
                                        <td>${u.phone}</td>
                                        <td>${u.disease}</td>
                                        <td>${u.date}</td>
                                        <td>
                                            <button class='action-btn edit-btn' onclick='editUser(${u.id})'>Edit</button>
                                            <button class='action-btn delete-btn' onclick='deleteUser(${u.id})'>Delete</button>
                                        </td>
                                     </tr>`;
                        }
                        html += "</table>";
                        document.getElementById("users").innerHTML = html;
                    }

                    window.onload = loadUsers;
                </script>
            </head>
            <body>
                <div class='top-right-icon'>
                <img src='https://cdn-icons-png.flaticon.com/512/3135/3135715.png' alt='icon'>
                </div>
                <div class='container'>
                    <h2>Hospital Management System</h2>
                    <div style='text-align:center; margin-top:20px;'>
                        <input type='text' id='name' placeholder='Enter name'>
                        <input type='text' id='phone' placeholder='Enter phone number'>
                        <input type='text' id='disease' placeholder='Enter disease'>
                        <input type='text' id='date' placeholder='Enter Appointment Date'>
                        <button onclick='addUser()'>Add Patient</button>
                    </div>

                    <hr>
                    <h3>All Patients</h3>
                    <div id='users'></div>
                </div>
            </body>
            </html>
        )";
        return crow::response(page.str());
    });
 // Add User
    CROW_ROUTE(app, "/add").methods("POST"_method)([db](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("name") || !body.has("phone") || !body.has("disease") || !body.has("date"))
            return crow::response(400, "Invalid input");

        std::string name = body["name"].s();
        std::string phone = body["phone"].s();
        std::string disease = body["disease"].s();
        std::string date = body["date"].s();

        std::string sql = "INSERT INTO users (name, phone, disease, date) VALUES (?, ?, ?, ?)";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, phone.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, disease.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return crow::response(200, "User added");
    });

    // Edit User
    CROW_ROUTE(app, "/edit").methods("POST"_method)([db](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("id") || !body.has("name") || !body.has("phone") || !body.has("disease") || !body.has("date"))
            return crow::response(400, "Invalid input");

        int id = body["id"].i();
        std::string name = body["name"].s();
        std::string phone = body["phone"].s();
        std::string disease = body["disease"].s();
        std::string date = body["date"].s();
        std::string sql = "UPDATE users SET name=?, phone=?, disease=?, date=? WHERE id=?";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, phone.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, disease.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return crow::response(200, "User updated");
    });

    // Delete User
    CROW_ROUTE(app, "/delete").methods("POST"_method)([db](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("id"))
            return crow::response(400, "Invalid input");

        int id = body["id"].i();

        std::string sql = "DELETE FROM users WHERE id=?";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return crow::response(200, "User deleted");
    });

    // Get all users
    CROW_ROUTE(app, "/users")([db]() {
        crow::json::wvalue result;
        crow::json::wvalue::list users;

        const char *sql = "SELECT id, name, phone, disease, date FROM users";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            crow::json::wvalue user;
            user["id"] = sqlite3_column_int(stmt, 0);
            user["name"] = (const char *)sqlite3_column_text(stmt, 1);
            user["phone"] = (const char *)sqlite3_column_text(stmt, 2);
            user["disease"] = (const char *)sqlite3_column_text(stmt, 3);
            user["date"] = (const char *)sqlite3_column_text(stmt, 4);


            users.push_back(user);
        }
        sqlite3_finalize(stmt);
        result = std::move(users);
        return crow::response(result);
    });

    app.port(3000).multithreaded().run();

    sqlite3_close(db);
}
 
