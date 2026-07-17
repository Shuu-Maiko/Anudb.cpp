import React, { useState } from "react";
import Terminal from "./components/Terminal.jsx";

export default function App() {
  const [activeDatabase, setActiveDatabase] = useState("");
  const [revision, setRevision] = useState(0);

  const handleDatabaseModified = () => {
    setRevision((prev) => prev + 1);
  };

  return (
    <>
      <div className="texture-overlay" aria-hidden="true"></div>
      <div className="app-container">
        <header className="brutalist-header">
          <div className="system-title">Anudb.cpp</div>
        </header>

        <main className="brutalist-main">
          <Terminal
            onDatabaseModified={handleDatabaseModified}
            activeDatabase={activeDatabase}
            setActiveDatabase={setActiveDatabase}
          />
        </main>

        <footer className="brutalist-footer">
          wsm terminal
        </footer>
      </div>
    </>
  );
}
