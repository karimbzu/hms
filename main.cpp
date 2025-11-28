#include "crow.h"
#include <sqlite3.h>
#include <mutex>
#include <string>
#include <filesystem>
#include <iostream>

using namespace std;

static constexpr const char* DB_PATH = "/app/data/hospital.db";

// Initialize DB and tables if missing
void init_db_if_needed() {
    std::filesystem::create_directories("/app/data");
    sqlite3* db = nullptr;
    if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        throw runtime_error("Unable to open database for initialization");
    }

    const char* sql = R"SQL(
CREATE TABLE IF NOT EXISTS doctors (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  specialty TEXT
);
CREATE TABLE IF NOT EXISTS patients (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  ailment TEXT,
  doctor_id INTEGER DEFAULT 0
);
)SQL";

    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        string e = err ? err : string("unknown");
        sqlite3_free(err);
        sqlite3_close(db);
        throw runtime_error("DB init failed: " + e);
    }
    sqlite3_close(db);
}

int main() {
    try {
        init_db_if_needed();
    } catch (const std::exception& ex) {
        cerr << "DB init error: " << ex.what() << "";
        // continue; health endpoint will report DB problems
    }

    crow::SimpleApp app;
    std::mutex db_mtx;

    // Embedded single-file SPA (safe delimiter)
    static const std::string index_html = R"CROWHTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Hospital CRUD - Crow</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
  <style>
    body { background: linear-gradient(135deg,#e0eafc 0%,#cfdef3 100%); min-height:100vh; padding:2rem; }
    .glass { background: rgba(255,255,255,0.6); backdrop-filter: blur(6px) saturate(120%); border-radius:12px; padding:1rem; }
    .table-small td, .table-small th { padding:.4rem; }
    #barChart { width:100%; height:320px; }
  </style>
</head>
<body>
<div class="container">
  <div class="glass p-4">
    <h1 class="mb-3">Hospital Management (Crow + C++)</h1>
    <ul class="nav nav-tabs" id="mainTabs" role="tablist">
      <li class="nav-item"><button class="nav-link active" data-bs-toggle="tab" data-bs-target="#doctors">Doctors</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#patients">Patients</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#charts">Charts</button></li>
    </ul>
    <div class="tab-content mt-3">
      <div class="tab-pane fade show active" id="doctors">
        <div class="d-flex justify-content-between align-items-center mb-2">
          <h4>Doctors</h4>
          <div>
            <button class="btn btn-sm btn-primary" onclick="addDoctor()">Add Doctor</button>
            <button class="btn btn-sm btn-secondary" onclick="loadDoctors()">Refresh</button>
          </div>
        </div>
        <table class="table table-striped table-small" id="doctorsTable"><thead><tr><th>ID</th><th>Name</th><th>Specialty</th><th>Actions</th></tr></thead><tbody></tbody></table>
      </div>
      <div class="tab-pane fade" id="patients">
        <div class="d-flex justify-content-between align-items-center mb-2">
          <h4>Patients</h4>
          <div>
            <button class="btn btn-sm btn-primary" onclick="addPatient()">Add Patient</button>
            <button class="btn btn-sm btn-secondary" onclick="loadPatients()">Refresh</button>
          </div>
        </div>
        <table class="table table-striped table-small" id="patientsTable"><thead><tr><th>ID</th><th>Name</th><th>Ailment</th><th>Doctor</th><th>Actions</th></tr></thead><tbody></tbody></table>
      </div>
      <div class="tab-pane fade" id="charts">
        <h4>Patients per Doctor</h4>
        <canvas id="barChart"></canvas>
        <div class="mt-2"><button class="btn btn-sm btn-secondary" onclick="renderChart()">Refresh Chart</button></div>
      </div>
    </div>
  </div>
</div>
<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<script>
async function api(path, method='GET', body=null){
  const opts = { method, headers: {} };
  if(body){ opts.headers['Content-Type']='application/json'; opts.body=JSON.stringify(body); }
  const res = await fetch(path, opts);
  if(res.status===204) return null;
  const txt = await res.text();
  try { return txt ? JSON.parse(txt) : null; } catch(e) { throw txt; }
}

// Doctors
async function loadDoctors(){
  try{
    const data = await api('/api/doctors');
    const tbody = document.querySelector('#doctorsTable tbody'); tbody.innerHTML='';
    if(!Array.isArray(data)) { console.error('doctors not array', data); return; }
    for(const d of data){
      const tr = document.createElement('tr');
      tr.innerHTML = `<td>${d.id}</td><td>${d.name}</td><td>${d.specialty}</td>` +
        `<td><button class='btn btn-sm btn-outline-primary' onclick='editDoctor(${d.id})'>Edit</button> `+
        `<button class='btn btn-sm btn-outline-danger' onclick='deleteDoctor(${d.id})'>Delete</button></td>`;
      tbody.appendChild(tr);
    }
  }catch(e){ alert('Error loading doctors: '+e); }
}
async function addDoctor(){ const name=prompt('Doctor name:'); if(!name) return; const specialty=prompt('Specialty:')||''; await api('/api/doctors','POST',{name,specialty}); loadDoctors(); renderChart(); }
async function editDoctor(id){ try{ const data=await api('/api/doctors/'+id); const name=prompt('Doctor name:',data.name); if(name===null) return; const specialty=prompt('Specialty:',data.specialty); if(specialty===null) return; await api('/api/doctors/'+id,'PUT',{name,specialty}); loadDoctors(); renderChart(); }catch(e){ alert('Error: '+e); } }
async function deleteDoctor(id){ if(!confirm('Delete doctor #'+id+'?')) return; await api('/api/doctors/'+id,'DELETE'); loadDoctors(); renderChart(); }

// Patients
async function loadPatients(){
  try{
    const data = await api('/api/patients');
    const tbody = document.querySelector('#patientsTable tbody'); tbody.innerHTML='';
    if(!Array.isArray(data)) { console.error('patients not array', data); return; }
    for(const p of data){
      const tr = document.createElement('tr');
      tr.innerHTML = `<td>${p.id}</td><td>${p.name}</td><td>${p.ailment}</td><td>${p.doctor_name||'Unassigned'}</td>` +
        `<td><button class='btn btn-sm btn-outline-primary' onclick='editPatient(${p.id})'>Edit</button> `+
        `<button class='btn btn-sm btn-outline-danger' onclick='deletePatient(${p.id})'>Delete</button></td>`;
      tbody.appendChild(tr);
    }
  }catch(e){ alert('Error loading patients: '+e); }
}
async function addPatient(){ const name=prompt('Patient name:'); if(!name) return; const ailment=prompt('Ailment:')||''; const doctor_id=parseInt(prompt('Assign doctor ID (leave blank for none):')||'0')||0; await api('/api/patients','POST',{name,ailment,doctor_id}); loadPatients(); renderChart(); }
async function editPatient(id){ try{ const data=await api('/api/patients/'+id); const name=prompt('Patient name:',data.name); if(name===null) return; const ailment=prompt('Ailment:',data.ailment); if(ailment===null) return; const doctor_id=parseInt(prompt('Assign doctor ID (0 for none):',data.doctor_id||0)||'0')||0; await api('/api/patients/'+id,'PUT',{name,ailment,doctor_id}); loadPatients(); renderChart(); }catch(e){ alert('Error: '+e); } }
async function deletePatient(id){ if(!confirm('Delete patient #'+id+'?')) return; await api('/api/patients/'+id,'DELETE'); loadPatients(); renderChart(); }

// Chart
let chart=null;
async function renderChart(){
  try{
    const json = await api('/api/chart');
    const ctx = document.getElementById('barChart').getContext('2d');
    if(chart) chart.destroy();
    chart = new Chart(ctx, {
      type: 'bar',
      data: { labels: json.labels || [], datasets: [{ label: 'Patients', data: json.counts || [] }] },
      options: { responsive:true, maintainAspectRatio:false }
    });
  }catch(e){ console.error(e); }
}

// init
loadDoctors(); loadPatients(); renderChart();
</script>
</body>
</html>
)CROWHTML";

    CROW_ROUTE(app, "/")( [] (const crow::request&){
        crow::response res;
        res.set_header("Content-Type","text/html; charset=utf-8");
        res.body = index_html;
        return res;
    });

    // Health endpoint
    CROW_ROUTE(app, "/health").methods(crow::HTTPMethod::GET)([&](const crow::request&){
        sqlite3* db = nullptr;
        int rc = sqlite3_open_v2(DB_PATH, &db, SQLITE_OPEN_READWRITE, nullptr);
        crow::json::wvalue out;
        if (rc == SQLITE_OK) { out["status"] = "ok"; sqlite3_close(db); return crow::response(out); }
        out["status"] = "db_error"; if (db) sqlite3_close(db); return crow::response(500);
    });

    // ---- Doctors CRUD ----
    CROW_ROUTE(app, "/api/doctors").methods(crow::HTTPMethod::GET)([&](const crow::request&){
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "SELECT id,name,specialty FROM doctors ORDER BY id";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        crow::json::wvalue arr; int idx = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            arr[idx]["id"] = sqlite3_column_int(stmt, 0);
            const unsigned char* n = sqlite3_column_text(stmt, 1);
            const unsigned char* s = sqlite3_column_text(stmt, 2);
            arr[idx]["name"] = n ? reinterpret_cast<const char*>(n) : string("");
            arr[idx]["specialty"] = s ? reinterpret_cast<const char*>(s) : string("");
            ++idx;
        }
        sqlite3_finalize(stmt); sqlite3_close(db);
        return crow::response(arr);
    });

    CROW_ROUTE(app, "/api/doctors/<int>").methods(crow::HTTPMethod::GET)([&](const crow::request&, int id){
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "SELECT id,name,specialty FROM doctors WHERE id = ?";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) != SQLITE_ROW) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(404); }
        crow::json::wvalue item; item["id"] = sqlite3_column_int(stmt, 0); item["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)); item["specialty"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(item);
    });

    CROW_ROUTE(app, "/api/doctors").methods(crow::HTTPMethod::POST)([&](const crow::request& req){
        auto body = crow::json::load(req.body); if (!body) return crow::response(400);
        const string name = body["name"].s(); const string specialty = body["specialty"].s();
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "INSERT INTO doctors(name,specialty) VALUES(?,?)";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, specialty.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(500); }
        sqlite3_finalize(stmt); sqlite3_close(db);
        crow::json::wvalue out; out["ok"] = true; return crow::response(201);
    });

    CROW_ROUTE(app, "/api/doctors/<int>").methods(crow::HTTPMethod::PUT)([&](const crow::request& req, int id){
        auto body = crow::json::load(req.body); if (!body) return crow::response(400);
        const string name = body["name"].s(); const string specialty = body["specialty"].s();
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "UPDATE doctors SET name=?,specialty=? WHERE id=?";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, specialty.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, id);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(500); }
        sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(200);
    });

    CROW_ROUTE(app, "/api/doctors/<int>").methods(crow::HTTPMethod::DELETE)([&](int id){
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "DELETE FROM doctors WHERE id = ?";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(500); }
        sqlite3_finalize(stmt);
        const char* sql2 = "UPDATE patients SET doctor_id = 0 WHERE doctor_id = ?";
        if (sqlite3_prepare_v2(db, sql2, -1, &stmt, nullptr) == SQLITE_OK) { sqlite3_bind_int(stmt, 1, id); sqlite3_step(stmt); sqlite3_finalize(stmt); }
        sqlite3_close(db);
        return crow::response(200);
    });

    // ---- Patients CRUD ----
    CROW_ROUTE(app, "/api/patients").methods(crow::HTTPMethod::GET)([&](const crow::request&){
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "SELECT p.id,p.name,p.ailment,p.doctor_id,d.name FROM patients p LEFT JOIN doctors d ON p.doctor_id=d.id ORDER BY p.id";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        crow::json::wvalue arr; int idx = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            arr[idx]["id"] = sqlite3_column_int(stmt, 0);
            arr[idx]["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            arr[idx]["ailment"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            arr[idx]["doctor_id"] = sqlite3_column_int(stmt, 3);
            const unsigned char* dn = sqlite3_column_text(stmt, 4);
            if (dn) arr[idx]["doctor_name"] = reinterpret_cast<const char*>(dn);
            ++idx;
        }
        sqlite3_finalize(stmt); sqlite3_close(db);
        return crow::response(arr);
    });

    CROW_ROUTE(app, "/api/patients/<int>").methods(crow::HTTPMethod::GET)([&](const crow::request&, int id){
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "SELECT id,name,ailment,doctor_id FROM patients WHERE id = ?";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) != SQLITE_ROW) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(404); }
        crow::json::wvalue item; item["id"] = sqlite3_column_int(stmt, 0); item["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)); item["ailment"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)); item["doctor_id"] = sqlite3_column_int(stmt, 3);
        sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(item);
    });

    CROW_ROUTE(app, "/api/patients").methods(crow::HTTPMethod::POST)([&](const crow::request& req){
        auto body = crow::json::load(req.body); if (!body) return crow::response(400);
        const string name = body["name"].s(); const string ailment = body["ailment"].s(); const int doctor_id = body["doctor_id"].i();
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "INSERT INTO patients(name,ailment,doctor_id) VALUES(?,?,?)";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, ailment.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, doctor_id);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(500); }
        sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(201);
    });

    CROW_ROUTE(app, "/api/patients/<int>").methods(crow::HTTPMethod::PUT)([&](const crow::request& req, int id){
        auto body = crow::json::load(req.body); if (!body) return crow::response(400);
        const string name = body["name"].s(); const string ailment = body["ailment"].s(); const int doctor_id = body["doctor_id"].i();
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "UPDATE patients SET name=?,ailment=?,doctor_id=? WHERE id=?";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, ailment.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, doctor_id);
        sqlite3_bind_int(stmt, 4, id);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(500); }
        sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(200);
    });

    CROW_ROUTE(app, "/api/patients/<int>").methods(crow::HTTPMethod::DELETE)([&](int id){
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "DELETE FROM patients WHERE id = ?";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(500); }
        sqlite3_finalize(stmt); sqlite3_close(db); return crow::response(200);
    });

    // ---- Chart endpoint ----
    CROW_ROUTE(app, "/api/chart").methods(crow::HTTPMethod::GET)([&](const crow::request&){
        std::lock_guard<std::mutex> lk(db_mtx);
        sqlite3* db = nullptr; if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) return crow::response(500);
        const char* sql = "SELECT d.name, COUNT(p.id) FROM doctors d LEFT JOIN patients p ON p.doctor_id=d.id GROUP BY d.id ORDER BY d.id";
        sqlite3_stmt* stmt = nullptr; if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_close(db); return crow::response(500); }
        crow::json::wvalue out; int i_labels = 0, i_counts = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* dn = sqlite3_column_text(stmt, 0);
            out["labels"][i_labels++] = dn ? reinterpret_cast<const char*>(dn) : string("");
            out["counts"][i_counts++] = sqlite3_column_int(stmt, 1);
        }
        sqlite3_finalize(stmt); sqlite3_close(db);
        return crow::response(out);
    });

    // Bind to 0.0.0.0 so other containers/host can reach it
    app.bindaddr("0.0.0.0").port(8080).multithreaded().run();
    return 0;
}
