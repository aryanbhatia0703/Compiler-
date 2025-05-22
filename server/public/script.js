// Tab Switching
document.addEventListener('DOMContentLoaded', () => {
  const tabs = document.querySelectorAll('nav button');
  const lexerSection = document.getElementById('lexerSection');
  const grammarSection = document.getElementById('grammarSection');

  // Initialize sections
  lexerSection.style.display = 'block';
  grammarSection.style.display = 'none';

  tabs.forEach((tab, index) => {
    tab.addEventListener('click', () => {
      // Update tab styles
      tabs.forEach(t => {
        t.classList.remove('tab-active');
        t.classList.add('text-gray-500');
      });
      tab.classList.add('tab-active');
      tab.classList.remove('text-gray-500');

      // Show/hide sections
      if (index === 0) {
        lexerSection.style.display = 'block';
        grammarSection.style.display = 'none';
      } else {
        lexerSection.style.display = 'none';
        grammarSection.style.display = 'block';
      }
    });
  });
});

// A mapping from raw token types to human-friendly display names
const tokenTypeMap = {
  "=": "assignment operator (=)",
  "+": "plus operator (+)",
  "-": "minus operator (-)",
  "*": "multiply operator (*)",
  "/": "divide operator (/)",
  "%": "modulus operator (%)",
  "==": "equality operator (==)",
  ";": "semicolon (;)",
  "(": "open parenthesis (",
  ")": "close parenthesis )",
  "{": "open curly brace {",
  "}": "close curly brace }",
  "int": "int (keyword)",
  "float": "float (keyword)",
  "void": "void (keyword)",
  "if": "if (keyword)",
  "else": "else (keyword)",
  "cin": "cin (keyword)",
  "cout": "cout (keyword)",
  "main": "main (keyword)",
  "identifier": "identifier",
  "integer constant": "integer constant",
  "float constant": "float constant",
  "error": "error"
};

// When the user clicks the "Run Lexer" button
document.getElementById('runBtn').addEventListener('click', async () => {
  const code = document.getElementById('codeInput').value;

  if (!code.trim()) {
    alert('Please enter some code.');
    return;
  }

  try {
    const response = await fetch('/run-lexer', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ code })
    });

    const data = await response.json();

    if (data.error) {
      alert('Error: ' + data.error);
      return;
    }

    populateTable(data.tokens);
  } catch (err) {
    console.error(err);
    alert('Something went wrong while contacting the server.');
  }
});

// This function displays tokens in the HTML table
function populateTable(tokens) {
  const tbody = document.getElementById('tokenTableBody');
  tbody.innerHTML = ''; // Clear previous

  tokens.forEach(token => {
    const row = document.createElement('tr');
    row.innerHTML = `
      <td class="border p-2">${tokenTypeMap[token.type] || token.type}</td>
      <td class="border p-2">${token.value}</td>
      <td class="border p-2">${token.line}</td>
      <td class="border p-2">${token.error ? '❌' : ''}</td>
    `;
    tbody.appendChild(row);
  });
}


//this function displays the grammer
document.getElementById("parseGrammarBtn").addEventListener("click", async () => {
  const grammar = document.getElementById("grammarInput").value;

  // Ensure proper formatting
  const formattedGrammar = grammar
    .split('\n')
    .map(line => line.trim())  // Remove leading/trailing whitespace
    .filter(line => line)      // Remove empty lines
    .join('\n');               // Join back with newlines

  // Add %end if not present
  const finalGrammar = formattedGrammar.endsWith('%end') ? formattedGrammar : formattedGrammar + '\n%end';

  try {
  const response = await fetch("/run-grammar", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ grammar: finalGrammar })
  });

  if (!response.ok) {
      const errorData = await response.json();
      alert(errorData.error || "Failed to parse grammar.");
    return;
  }

  const data = await response.json();
 const tbody = document.getElementById("grammarTableBody");
tbody.innerHTML = "";

    // Handle the productions array from the JSON structure
    data.productions.forEach(rule => {
  const row = document.createElement("tr");

  const parentCell = document.createElement("td");
  parentCell.className = "px-4 py-2 border font-medium text-blue-700";
  parentCell.textContent = rule.parent;

  const rulesCell = document.createElement("td");
  rulesCell.className = "px-4 py-2 border";
  rulesCell.innerHTML = rule.rules.map(r => r.join(" ")).join("<br>");

  row.appendChild(parentCell);
  row.appendChild(rulesCell);
  tbody.appendChild(row);
});
  } catch (err) {
    console.error("Error:", err);
    alert("Error processing grammar: " + err.message);
  }
});

//
document.getElementById("generateFirstFollow").addEventListener("click", () => {
  fetch("/first-follow")
    .then((res) => res.json())
    .then((data) => {
      renderFirstFollow(data);
    })
    .catch((err) => {
      console.error("Error fetching first_follow.json:", err);
    });
});

function renderFirstFollow(data) {
  const container = document.getElementById('firstFollowContainer');
  container.innerHTML = `
    <div class="space-y-6">
      <div>
        <h3 class="text-lg font-medium text-gray-900 mb-3">FIRST Sets</h3>
        ${generateTable(data.first)}
      </div>
      <div>
        <h3 class="text-lg font-medium text-gray-900 mb-3">FOLLOW Sets</h3>
        ${generateTable(data.follow)}
      </div>
    </div>
  `;
}

function generateTable(map) {
  let html = `
    <div class="overflow-x-auto rounded-lg border border-gray-200">
      <table class="min-w-full divide-y divide-gray-200">
        <thead class="bg-gray-50">
          <tr>
            <th class="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Non-terminal</th>
            <th class="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Set</th>
          </tr>
        </thead>
        <tbody class="bg-white divide-y divide-gray-200">
  `;
  for (const [nt, symbols] of Object.entries(map)) {
    html += `
      <tr>
        <td class="px-4 py-3 text-sm">${nt}</td>
        <td class="px-4 py-3 text-sm">${symbols.join(", ")}</td>
      </tr>
    `;
  }
  html += "</tbody></table></div>";
  return html;
}


//parsing table

document.getElementById("generateParsingTable").addEventListener("click", () => {
  fetch("/parsing-table")
    .then((res) => res.json())
    .then((data) => {
      renderParsingTable(data);
    })
    .catch((err) => {
      console.error("Error fetching parsing_table.json:", err);
    });
});

function renderParsingTable(data) {
  const container = document.getElementById('parsingTableContainer');
  container.innerHTML = generateParsingTableHTML(data);
}

function generateParsingTableHTML(table) {
  let terminals = new Set();
  for (const nonTerminal in table) {
    for (const terminal in table[nonTerminal]) {
      terminals.add(terminal);
    }
  }
  terminals = Array.from(terminals);

  let html = `
    <div class="overflow-x-auto rounded-lg border border-gray-200">
      <table class="min-w-full divide-y divide-gray-200">
        <thead class="bg-gray-50">
          <tr>
            <th class="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Non-terminal</th>
            ${terminals.map(t => `<th class="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">${t}</th>`).join('')}
          </tr>
        </thead>
        <tbody class="bg-white divide-y divide-gray-200">
  `;

  for (const nt in table) {
    html += `<tr>
      <td class="px-4 py-3 text-sm font-medium">${nt}</td>`;
    for (const t of terminals) {
      const rule = table[nt][t] || "";
      html += `<td class="px-4 py-3 text-sm">${rule}</td>`;
    }
    html += "</tr>";
  }

  html += "</tbody></table></div>";
  return html;
}

// LL(1) Parser Functionality

// Run Parser button event listener
document.getElementById('runParser').addEventListener('click', async () => {
  try {
    // Show a loading indicator
    const button = document.getElementById('runParser');
    const originalText = button.textContent;
    button.textContent = 'Parsing...';
    button.disabled = true;
    
    // Get tokens from the token table if available
    const tokens = [];
    const tokenRows = document.getElementById('tokenTableBody').querySelectorAll('tr');
    
    if (tokenRows.length > 0) {
      tokenRows.forEach(row => {
        const cells = row.querySelectorAll('td');
        // Only include valid tokens (not errors)
        if (cells.length >= 3 && !cells[3].textContent.includes('❌')) {
          // Extract the token type from the display text
          let displayType = cells[0].textContent;
          let tokenType = displayType;
          
          // Handle parenthesized token types like "identifier (id)" -> "id"
          if (displayType.includes('(') && displayType.includes(')')) {
            // Extract the original type from parentheses if available
            const match = displayType.match(/\(([^)]+)\)/);
            if (match && match[1]) {
              tokenType = match[1];
            }
          } else if (displayType.includes(' ')) {
            // For simple descriptions like "plus operator (+)" -> "+"
            const match = displayType.match(/([+\-*\/(){}[\],;=%])/);
            if (match && match[1]) {
              tokenType = match[1];
            } else {
              // Just use the first word for keywords etc.
              tokenType = displayType.split(' ')[0];
            }
          }
          
          tokens.push({
            type: tokenType,
            value: cells[1].textContent,
            line: parseInt(cells[2].textContent) || 1
          });
        }
      });
    }
    
    console.log("Sending tokens to parser:", tokens);
    
    // Call the parser API
    const response = await fetch('/run-parser', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ tokens })
    });
    
    const data = await response.json();
    
    // Always refresh the table first if we have trace data
    if (data.parseTrace && data.parseTrace.length > 0) {
      displayParserResults(data);
    } else {
      alert('No parsing trace was generated.');
      button.textContent = originalText;
      button.disabled = false;
      return;
    }
    
    // After table is refreshed, show any error messages
    if (data.error) {
      alert('Parsing Error: ' + data.error);
    } else if (data.success === false && data.message) {
      alert('Warning: ' + data.message);
    }
    
    // Reset the button
    button.textContent = originalText;
    button.disabled = false;
    
  } catch (err) {
    console.error('Parser error:', err);
    alert('Error during parsing: ' + err.message);
    document.getElementById('runParser').textContent = 'Parse Input';
    document.getElementById('runParser').disabled = false;
  }
});

// Display the parser results
function displayParserResults(data) {
  const resultsContainer = document.getElementById('parserResults');
  resultsContainer.classList.remove('hidden');
  
  // Display parse trace
  displayParseTrace(data.parseTrace);
  
  // Display parse tree
  displayParseTree(data.parseTree);
}

// Display the parse trace in a table
function displayParseTrace(trace) {
  const traceBody = document.getElementById('parseTraceBody');
  traceBody.innerHTML = '';
  
  trace.forEach((entry, index) => {
    const row = document.createElement('tr');
    
    // Apply different styling based on action type
    let rowClass = '';
    let actionClass = '';
    
    if (entry.action === 'match') {
      rowClass = 'bg-green-50';
      actionClass = 'text-green-600 font-semibold';
    } else if (entry.action === 'accept') {
      rowClass = 'bg-blue-50';
      actionClass = 'text-blue-600 font-semibold';
    } else if (entry.action.startsWith('expand:')) {
      rowClass = 'bg-yellow-50';
      actionClass = 'text-yellow-600';
    } else if (entry.action.startsWith('error:')) {
      rowClass = 'bg-red-50';
      actionClass = 'text-red-600 font-semibold';
    }
    
    row.className = rowClass;
    row.innerHTML = `
      <td class="border p-2">${index + 1}</td>
      <td class="border p-2">${entry.stack_top}</td>
      <td class="border p-2">${entry.input}</td>
      <td class="border p-2 ${actionClass}">${entry.action}</td>
    `;
    
    traceBody.appendChild(row);
  });
}

// Display the parse tree
function displayParseTree(treeData) {
  const treeContainer = document.getElementById('parseTreeContainer');
  treeContainer.innerHTML = '';
  
  // Generate HTML for the tree
  const treeHtml = generateTreeHTML(treeData);
  treeContainer.innerHTML = treeHtml;
}

// Recursively generate HTML for the tree
function generateTreeHTML(node, indent = 0) {
  const indentSize = 20; // Pixels per indent level
  const indentPx = indent * indentSize;
  
  // Style based on whether it's a terminal or non-terminal
  const isTerminal = node.children.length === 0;
  const nodeStyle = isTerminal 
    ? 'text-green-600 font-semibold' 
    : 'text-purple-700 font-bold';
  
  let html = `
    <div class="tree-node" style="margin-left: ${indentPx}px; margin-bottom: 4px;">
      <span class="${nodeStyle}">${node.symbol}</span>
    </div>
  `;
  
  // Generate children
  if (node.children && node.children.length > 0) {
    const childrenHtml = node.children.map(child => 
      generateTreeHTML(child, indent + 1)
    ).join('');
    
    html += `
      <div class="children" style="margin-left: ${indentPx + 20}px; border-left: 1px dashed #ccc; padding-left: 4px;">
        ${childrenHtml}
      </div>
    `;
  }
  
  return html;
}



