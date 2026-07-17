import React, { useState, useEffect, useRef } from "react";

function FastFetchCard() {
  const dotColors = ["#c5b3ab", "#e8c4b7", "#d28b7c", "#70c5c6", "#5897c8", "#b298cf", "#8fbcd4", "#7a8ba3"];
  return (
    <div className="fastfetch-card">
      <div className="fastfetch-left">
        <img src="/shuu.png" alt="shuu" className="fastfetch-avatar" />
      </div>
      <div className="fastfetch-right">
        <div className="fastfetch-header">priyanshu@anudb</div>
        <div className="fastfetch-divider">-----------------</div>
        <div className="fastfetch-row"><span className="fastfetch-label">user</span>:      Priyanshu Negi</div>
        <div className="fastfetch-row"><span className="fastfetch-label">benchmark done </span>:    1.2M insertions/sec (native speed)</div>
        <div className="fastfetch-row"><span className="fastfetch-label">base data structure</span>:       B+Tree Index Processor</div>
        <div className="fastfetch-row"><span className="fastfetch-label">pagging size </span>:      4096B Block Page Mapping</div>
        <div className="fastfetch-row"><span className="fastfetch-label">memory: managment by </span>:    mmap()</div>
        <div className="fastfetch-row"><span className="fastfetch-label">made by</span>:   Priyanshu Negi</div>
        <div className="fastfetch-row"><span className="fastfetch-label">github </span>: <a href="https://github.com/Shuu-Maiko/Anudb.cpp" target="_blank" rel="noreferrer" className="fastfetch-link">github.com/shuu-maiko/anudb.cpp</a></div>
        <div className="fastfetch-row">
          <span className="fastfetch-label">colors</span>:     {" "}
          {dotColors.map((color, i) => (
            <span key={i} style={{ color, marginRight: "2px" }}>●</span>
          ))}
        </div>
      </div>
    </div>
  );
}

export default function Terminal({ onDatabaseModified, activeDatabase, setActiveDatabase }) {
  const [input, setInput] = useState("");
  const [logs, setLogs] = useState([
    "types : int, text, float",
    "ddl keyword : create table, create database, use",
    "dml keyword : select, insert, update, delete",
    "extra: fastfetch"
  ]);
  const [wasmLoaded, setWasmLoaded] = useState(false);
  const [history, setHistory] = useState([]);
  const [historyIndex, setHistoryIndex] = useState(-1);
  const [isPrinting, setIsPrinting] = useState(false);
  const bodyRef = useRef(null);

  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

  const presets = [
    {
      id: 1,
      label: "schema",
      sql: "CREATE DATABASE demo;\nUSE demo;\nCREATE TABLE users (id INT, name TEXT, age INT);"
    },
    {
      id: 2,
      label: "insert",
      sql: "INSERT INTO users VALUES (1, 'Alice', 30);\nINSERT INTO users VALUES (2, 'Bob', 25);"
    },
    {
      id: 3,
      label: "select",
      sql: "SELECT * FROM users;"
    },
    {
      id: 4,
      label: "filter",
      sql: "SELECT name, age FROM users WHERE id = 1;"
    },
    {
      id: 5,
      label: "update",
      sql: "UPDATE users SET age = 31 WHERE id = 1;\nSELECT * FROM users;"
    },
    {
      id: 6,
      label: "delete",
      sql: "DELETE FROM users WHERE id = 2;\nSELECT * FROM users;"
    }
  ];

  useEffect(() => {
    if (window.Module) {
      if (window.Module.calledRun || window.Module.asm) {
        window.Module.ccall("init_db", null, [], []);
        setWasmLoaded(true);
        return;
      }
    }

    window.Module = {
      onRuntimeInitialized: () => {
        window.Module.ccall("init_db", null, [], []);
        setWasmLoaded(true);
      }
    };

    const script = document.createElement("script");
    script.src = "/anudb.js";
    script.async = true;
    document.body.appendChild(script);

    return () => {
      document.body.removeChild(script);
    };
  }, []);

  useEffect(() => {
    const handleGlobalKeyDown = (e) => {
      if (isPrinting || !wasmLoaded) return;
      if (document.activeElement.tagName === "INPUT" || document.activeElement.tagName === "TEXTAREA") {
        return;
      }
      if (e.key >= "1" && e.key <= "6") {
        e.preventDefault();
        const index = parseInt(e.key) - 1;
        runPreset(presets[index].sql);
      }
    };

    window.addEventListener("keydown", handleGlobalKeyDown);
    return () => {
      window.removeEventListener("keydown", handleGlobalKeyDown);
    };
  }, [wasmLoaded, activeDatabase, isPrinting]);

  useEffect(() => {
    if (bodyRef.current) {
      bodyRef.current.scrollTop = bodyRef.current.scrollHeight;
    }
  }, [logs]);

  const executeCommand = async (command) => {
    if (!wasmLoaded) {
      setLogs((prev) => [...prev, "sys.err // wasm_heap_pending"]);
      return;
    }

    const trimmed = command.trim();
    if (!trimmed) return;

    setIsPrinting(true);
    setLogs((prev) => [...prev, `${getPrompt()}${command}`]);

    if (trimmed.toLowerCase() === "fastfetch") {
      await sleep(100);
      setLogs((prev) => [...prev, "SYS.FASTFETCH.BLOCK"]);
      setIsPrinting(false);
      onDatabaseModified();
      return;
    }

    if (trimmed.toLowerCase().startsWith("use ")) {
      const parts = trimmed.split(/\s+/);
      if (parts.length > 1) {
        const dbName = parts[1].replace(";", "");
        setActiveDatabase(dbName);
      }
    }

    try {
      const output = window.Module.ccall("execute_sql", "string", ["string"], [trimmed]);
      const lines = output.toLowerCase().split("\n");
      for (const line of lines) {
        if (line.trim()) {
          await sleep(40);
          setLogs((prev) => [...prev, line]);
        }
      }
      onDatabaseModified();
    } catch (error) {
      setLogs((prev) => [...prev, `sys.err // internal_exec_fault: ${error.message}`]);
    } finally {
      setIsPrinting(false);
    }
  };

  const handleKeyDown = (e) => {
    if (isPrinting) return;
    if (e.key === "Enter") {
      executeCommand(input);
      setHistory((prev) => [...prev, input]);
      setHistoryIndex(-1);
      setInput("");
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      if (history.length === 0) return;
      const nextIndex = historyIndex === -1 ? history.length - 1 : Math.max(0, historyIndex - 1);
      setHistoryIndex(nextIndex);
      setInput(history[nextIndex]);
    } else if (e.key === "ArrowDown") {
      e.preventDefault();
      if (historyIndex === -1) return;
      const nextIndex = historyIndex + 1;
      if (nextIndex >= history.length) {
        setHistoryIndex(-1);
        setInput("");
      } else {
        setHistoryIndex(nextIndex);
        setInput(history[nextIndex]);
      }
    }
  };

  const getPrompt = () => {
    return activeDatabase ? `anudb [${activeDatabase.toLowerCase()}] >> ` : "anudb >> ";
  };

  const runPreset = async (sql) => {
    setIsPrinting(true);
    const commands = sql.split("\n").filter(line => line.trim());
    for (const cmd of commands) {
      await executeCommand(cmd);
      await sleep(120);
    }
    setIsPrinting(false);
  };

  const formatLogLine = (log) => {
    const tokens = log.split(/(\s+|\b)/);
    return tokens.map((token, idx) => {
      const lower = token.toLowerCase();
      if (["create", "table", "database", "use", "select", "insert", "update", "delete", "where", "values", "fastfetch", "extra"].includes(lower)) {
        return <span key={idx} className="keyword-highlight">{token}</span>;
      }
      if (["int", "text", "float"].includes(lower)) {
        return <span key={idx} className="type-highlight">{token}</span>;
      }
      if (["ok", "active", "loaded"].includes(lower)) {
        return <span key={idx} className="status-highlight">{token}</span>;
      }
      if (/^[1-6]$/.test(token)) {
        return <span key={idx} className="bright-num">{token}</span>;
      }
      return token;
    });
  };

  return (
    <div className="brutalist-console-layout">
      <div className="brutalist-terminal-container">
        <div className="terminal-body" ref={bodyRef}>
          {logs.map((log, index) => {
            if (log === "SYS.FASTFETCH.BLOCK") {
              return <FastFetchCard key={index} />;
            }
            return (
              <div key={index} className="terminal-output-line">
                {formatLogLine(log)}
              </div>
            );
          })}
          <div className="terminal-input-line">
            <span className="terminal-prompt">{getPrompt()}</span>
            <input
              type="text"
              className="terminal-input"
              value={input}
              onChange={(e) => setInput(e.target.value)}
              onKeyDown={handleKeyDown}
              disabled={!wasmLoaded}
              placeholder={wasmLoaded ? "sys.input..." : "sys.ldr.pending..."}
              autoFocus
            />
          </div>
        </div>
      </div>

      <div className="brutalist-sidebar">
        <div className="sidebar-title">sys.presets</div>
        {presets.map((preset) => (
          <button
            key={preset.id}
            className="brutalist-btn"
            onClick={() => runPreset(preset.sql)}
          >
            <span className="bright-num">{preset.id}</span>.{preset.label}
          </button>
        ))}
      </div>
    </div>
  );
}
