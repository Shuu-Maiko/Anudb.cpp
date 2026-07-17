import React, { useState, useEffect } from "react";

export default function HexViewer({ activeDatabase, revision }) {
  const [pages, setPages] = useState([]);
  const [selectedPageIndex, setSelectedPageIndex] = useState(0);

  useEffect(() => {
    if (!activeDatabase || !window.Module || !window.Module.FS) {
      setPages([]);
      return;
    }

    const filename = `/${activeDatabase}.anudb_data`;
    let exists = false;

    try {
      exists = window.Module.FS.analyzePath(filename).exists;
    } catch (e) {
      exists = false;
    }

    if (!exists) {
      setPages([]);
      return;
    }

    try {
      const data = window.Module.FS.readFile(filename);
      const pageSize = 4096;
      const totalPages = Math.ceil(data.length / pageSize);
      const newPages = [];

      for (let i = 0; i < totalPages; i++) {
        const start = i * pageSize;
        const end = Math.min(start + pageSize, data.length);
        newPages.push(data.slice(start, end));
      }

      setPages(newPages);
      if (selectedPageIndex >= newPages.length) {
        setSelectedPageIndex(0);
      }
    } catch (err) {
      setPages([]);
    }
  }, [activeDatabase, revision, selectedPageIndex]);

  const renderHexRows = (pageBytes) => {
    const rows = [];
    const bytesPerRow = 16;

    for (let i = 0; i < pageBytes.length; i += bytesPerRow) {
      const rowBytes = pageBytes.slice(i, i + bytesPerRow);
      const offsetStr = i.toString(16).padStart(4, "0").toUpperCase();
      const hexParts = [];
      const asciiParts = [];

      for (let j = 0; j < bytesPerRow; j++) {
        if (j < rowBytes.length) {
          const byte = rowBytes[j];
          const hex = byte.toString(16).padStart(2, "0").toUpperCase();
          const isMagic = selectedPageIndex === 0 && i === 0 && j < 4;
          const isVersion = selectedPageIndex === 0 && i === 0 && j >= 4 && j < 8;

          let className = "hex-byte";
          if (isMagic) className += " header";
          else if (isVersion) className += " active";

          hexParts.push(
            <span key={j} className={className}>
              {hex}
            </span>
          );

          const char = byte >= 32 && byte <= 126 ? String.fromCharCode(byte) : ".";
          asciiParts.push(char);
        } else {
          hexParts.push(<span key={j} className="hex-byte">&nbsp;&nbsp;</span>);
          asciiParts.push(" ");
        }
      }

      rows.push(
        <div key={i} className="hex-row">
          <span className="hex-offset">{offsetStr}</span>
          <span className="hex-bytes">{hexParts}</span>
          <span className="hex-ascii">{asciiParts.join("")}</span>
        </div>
      );
    }
    return rows;
  };

  if (!activeDatabase || pages.length === 0) {
    return (
      <div className="hex-window">
        <div className="window-header">
          <div className="window-dots">
            <div className="window-dot dot-red"></div>
            <div className="window-dot dot-yellow"></div>
            <div className="window-dot dot-green"></div>
          </div>
          <div className="window-title">Disk Page Inspector</div>
        </div>
        <div className="hex-body" style={{ display: "flex", alignItems: "center", justifyContent: "center", color: "var(--text-secondary)" }}>
          <div>No active database files created. Create one in the terminal to inspect raw disk pages.</div>
        </div>
      </div>
    );
  }

  return (
    <div className="hex-window">
      <div className="window-header">
        <div className="window-dots">
          <div className="window-dot dot-red"></div>
          <div className="window-dot dot-yellow"></div>
          <div className="window-dot dot-green"></div>
        </div>
        <div className="window-title">
          {activeDatabase}.anudb_data ({pages.length} Pages)
        </div>
      </div>
      <div style={{ padding: "0.5rem 1rem", background: "rgba(255,255,255,0.02)", borderBottom: "1px solid var(--border-color)", display: "flex", gap: "0.5rem", alignItems: "center", flexWrap: "wrap" }}>
        <span style={{ fontSize: "0.8rem", color: "var(--text-secondary)", marginRight: "0.5rem" }}>Select Page:</span>
        {pages.map((_, idx) => (
          <button
            key={idx}
            onClick={() => setSelectedPageIndex(idx)}
            className="btn"
            style={{
              padding: "0.2rem 0.5rem",
              fontSize: "0.75rem",
              background: selectedPageIndex === idx ? "var(--accent-color)" : "rgba(255,255,255,0.05)",
              color: selectedPageIndex === idx ? "#000" : "var(--text-secondary)",
              fontWeight: selectedPageIndex === idx ? "bold" : "normal"
            }}
          >
            Page {idx} {idx === 0 ? "(Header)" : idx === 1 ? "(Root)" : ""}
          </button>
        ))}
      </div>
      <div className="hex-body">
        {renderHexRows(pages[selectedPageIndex])}
      </div>
    </div>
  );
}
