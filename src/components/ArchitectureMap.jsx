import React, { useState } from "react";

export default function ArchitectureMap() {
  const [activeNode, setActiveNode] = useState(null);

  const nodes = [
    {
      id: "input",
      icon: "CLI",
      title: "Stateless REPL (main.cpp)",
      desc: "Takes user command strings, handles terminal history, and triggers execution cycles.",
      files: ["src/main.cpp", "include/Context.h"]
    },
    {
      id: "tokenizer",
      icon: "LEX",
      title: "SQL Tokenizer (Tokenizer.cpp)",
      desc: "Performs lexical analysis, converting raw query strings into structured keyword, literal, and symbol tokens.",
      files: ["src/Tokenizer.cpp", "include/Tokenizer.h"]
    },
    {
      id: "parser",
      icon: "AST",
      title: "Recursive Descent Parser (Parser.cpp)",
      desc: "Translates tokens into a polymorphic AST (Abstract Syntax Tree) representing statements like SELECT, INSERT, or CREATE.",
      files: ["src/Parser.cpp", "include/Parser.h"]
    },
    {
      id: "executor",
      icon: "EXE",
      title: "Query Executor (Executor.cpp)",
      desc: "Routes statements to metadata managers and storage engines, validating fields and executing actions.",
      files: ["src/Executor.cpp", "include/Executor.h"]
    },
    {
      id: "bptree",
      icon: "B+T",
      title: "Clustered B+Tree Storage (BPlusTree.cpp)",
      desc: "Organizes row data in nodes, splitting and merging pages to support point queries and range scans.",
      files: ["src/BPlusTree.cpp", "include/BPlusTree.h", "src/RowSerializer.cpp"]
    },
    {
      id: "pagemgr",
      icon: "MAP",
      title: "Memory-Mapped I/O (PageManager.cpp)",
      desc: "Maintains virtual page arrays using POSIX mmap, ensuring fast reads and SSD block-aligned page allocation.",
      files: ["src/PageManager.cpp", "include/PageManager.h"]
    }
  ];

  return (
    <div className="architecture-section">
      <h2 className="section-title">Database System Architecture</h2>
      <p className="section-desc">
        Hover or click on any module in the pipeline to explore how queries traverse from raw text to binary disk pages.
      </p>

      <div className="playground-grid" style={{ gridTemplateColumns: "1fr 1fr", alignItems: "start" }}>
        <div className="flow-container">
          {nodes.map((node) => (
            <div
              key={node.id}
              className="flow-node"
              style={{
                borderColor: activeNode?.id === node.id ? "var(--accent-color)" : "var(--border-color)",
                cursor: "pointer",
                boxShadow: activeNode?.id === node.id ? "0 0 15px var(--accent-glow)" : "none"
              }}
              onMouseEnter={() => setActiveNode(node)}
              onClick={() => setActiveNode(node)}
            >
              <div className="flow-icon">{node.icon}</div>
              <div className="flow-content">
                <div className="flow-title">{node.title}</div>
                <div className="flow-desc">{node.desc}</div>
              </div>
            </div>
          ))}
        </div>

        <div
          className="stat-card"
          style={{
            minHeight: "350px",
            display: "flex",
            flexDirection: "column",
            justifyContent: "center",
            textAlign: "left",
            position: "sticky",
            top: "2rem",
            background: "#080a10"
          }}
        >
          {activeNode ? (
            <div>
              <div style={{ display: "flex", alignItems: "center", gap: "1rem", marginBottom: "1.5rem" }}>
                <div className="flow-icon" style={{ margin: 0 }}>{activeNode.icon}</div>
                <h3 style={{ fontSize: "1.4rem", fontWeight: "600" }}>{activeNode.title}</h3>
              </div>
              <p style={{ color: "var(--text-secondary)", marginBottom: "2rem", fontSize: "1rem", lineHeight: "1.6" }}>
                {activeNode.desc}
              </p>
              <div>
                <h4 style={{ fontSize: "0.9rem", color: "var(--text-primary)", marginBottom: "0.75rem", textTransform: "uppercase", letterSpacing: "0.05em" }}>
                  Key Source Files:
                </h4>
                <div style={{ display: "flex", flexDirection: "column", gap: "0.5rem" }}>
                  {activeNode.files.map((file, i) => (
                    <code
                      key={i}
                      style={{
                        fontFamily: "var(--font-mono)",
                        background: "rgba(255,255,255,0.05)",
                        padding: "0.4rem 0.8rem",
                        borderRadius: "4px",
                        fontSize: "0.85rem",
                        border: "1px solid var(--border-color)"
                      }}
                    >
                      {file}
                    </code>
                  ))}
                </div>
              </div>
            </div>
          ) : (
            <div style={{ textAlign: "center", color: "var(--text-secondary)" }}>
              Hover over an architecture component on the left to inspect its implementation files and responsibility.
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
