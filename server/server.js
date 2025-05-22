const express = require('express');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');

const app = express();

// Use EJS as the view engine
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

// Serve static files like script.js from /public
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Define paths
const backendPath = path.join(__dirname, '..', 'backend');
const buildPath = path.join(backendPath, 'build');

// Clean up duplicate parse trace files from backend directory (not build directory)
const mainDirParseTrace = path.join(backendPath, 'parse_trace.json');
const mainDirParseTraceTxt = path.join(backendPath, 'parse_trace.txt');

// Remove any parse trace files from the main backend directory
// We only want them in the build directory
if (fs.existsSync(mainDirParseTrace)) {
  try {
    fs.unlinkSync(mainDirParseTrace);
    console.log("Removed duplicate parse_trace.json from backend directory");
  } catch (err) {
    console.error("Failed to remove duplicate parse_trace.json:", err);
  }
}

if (fs.existsSync(mainDirParseTraceTxt)) {
  try {
    fs.unlinkSync(mainDirParseTraceTxt);
    console.log("Removed duplicate parse_trace.txt from backend directory");
  } catch (err) {
    console.error("Failed to remove duplicate parse_trace.txt:", err);
  }
}

// Function to ensure the parser is built
function buildParser() {
  return new Promise((resolve, reject) => {
    const backendPath = path.join(__dirname, '..', 'backend');
    const exePath = path.join(backendPath, 'build', 'parser_run.exe');
    
    // Check if the parser is already built
    if (fs.existsSync(exePath)) {
      return resolve();
    }
    
    console.log("Parser executable not found. Building parser...");
    
    // Run the build script
    exec('powershell -ExecutionPolicy Bypass -File build_simple.ps1', 
      { cwd: backendPath }, 
      (error, stdout, stderr) => {
        if (error) {
          console.error("Error building parser:", error);
          console.error("Build stderr:", stderr);
          return reject(new Error("Failed to build parser. See server logs for details."));
        }
        
        console.log("Parser build output:", stdout);
        
        // Check if the build was successful
        if (fs.existsSync(exePath)) {
          console.log("Parser successfully built!");
          resolve();
        } else {
          console.error("Parser executable not found after build.");
          reject(new Error("Parser build appeared to succeed but executable not found."));
        }
      }
    );
  });
}

// Route to render the main page
app.get('/', (req, res) => {
  res.render('index');
});

// Route to handle lexer execution
app.post('/run-lexer', (req, res) => {
  const backendPath = path.join(__dirname, '..', 'backend');
  const exePath = path.join(backendPath, 'lexer_run.exe');
  const inputFilePath = path.join(backendPath, 'input.txt');
  const tokensPath = path.join(backendPath, 'tokens.json');
  const testTokensPath = path.join(backendPath, 'test_input_tokens.json');
  const buildDirPath = path.join(backendPath, 'build');
  const buildTestTokensPath = path.join(buildDirPath, 'test_input_tokens.json');

  const { code } = req.body;

  if (!code || code.trim() === '') {
    return res.status(400).json({ error: 'No code provided.' });
  }

  // Step 1: Write user input to input.txt
  fs.writeFileSync(inputFilePath, code);

  // Step 2: Run the lexer with input file
  exec(`"${exePath}" "${inputFilePath}"`, { cwd: backendPath }, (error, stdout, stderr) => {
      if (error) {
        console.error('Lexer execution error:', error);
        console.error('Stderr:', stderr);
        return res.status(500).json({ error: 'Lexer execution failed: ' + stderr });
      }

    // Step 3: Check if tokens.json exists and read it
    if (!fs.existsSync(tokensPath)) {
      console.error('tokens.json not found');
      return res.status(500).json({ error: 'Lexer did not generate output file.' });
    }

    try {
      const data = fs.readFileSync(tokensPath, 'utf8');
      const tokens = JSON.parse(data);
      
      // Also save a copy to test_input_tokens.json for the parser
      fs.writeFileSync(testTokensPath, data);
      console.log("Tokens copied to test_input_tokens.json");
      
      // Also copy to build directory if it exists
      if (fs.existsSync(buildDirPath)) {
        fs.writeFileSync(buildTestTokensPath, data);
        console.log("Tokens copied to build/test_input_tokens.json");
      }
      
      res.json({ tokens });
    } catch (e) {
      console.error('Error reading/parsing tokens.json:', e);
      res.status(500).json({ error: 'Failed to read lexer output: ' + e.message });
    }
  });
});

//Route to handle grammar parsing
app.post("/run-grammar", (req, res) => {
  const grammarText = req.body.grammar;
  console.log("Received grammar text (length):", grammarText.length);
  console.log("Grammar text (exact):\n'" + grammarText + "'");
  
  const backendPath = path.join(__dirname, '..', 'backend');
  const exePath = path.join(backendPath, 'grammar_run.exe');
  const grammarFilePath = path.join(backendPath, 'grammar.txt');
  const grammarJsonPath = path.join(backendPath, 'grammar.json');
  const testGrammarJsonPath = path.join(backendPath, 'test_grammar.json');
  const buildDirPath = path.join(backendPath, 'build');
  const buildTestGrammarPath = path.join(buildDirPath, 'test_grammar.json');

  // Clean up any existing files
  try {
    if (fs.existsSync(grammarFilePath)) {
      fs.unlinkSync(grammarFilePath);
    }
    if (fs.existsSync(grammarJsonPath)) {
      fs.unlinkSync(grammarJsonPath);
    }
  } catch (err) {
    console.error("Error cleaning up files:", err);
  }

  // Save custom grammar input to grammar.txt
  try {
    // Ensure the grammar text ends with a newline and %end
    let formattedGrammar = grammarText.trim();
    if (!formattedGrammar.endsWith('%end')) {
      formattedGrammar += '\n%end';
    }
    fs.writeFileSync(grammarFilePath, formattedGrammar);
    console.log("Grammar file written (length):", formattedGrammar.length);
    console.log("Grammar file contents:\n'" + formattedGrammar + "'");
  } catch (err) {
    console.error("Error writing grammar file:", err);
    return res.status(500).json({ error: "Failed to write grammar file" });
  }

  // Run grammar parser executable
  exec(`"${exePath}"`, { cwd: backendPath }, (error, stdout, stderr) => {
    console.log("Grammar parser stdout:", stdout);
    console.log("Grammar parser stderr:", stderr);
    if (error) {
      console.error("Parser error:", stderr);
      return res.status(500).json({ error: "Failed to parse grammar: " + stderr });
    }

    // Read the resulting grammar.json
    try {
      const grammarData = fs.readFileSync(grammarJsonPath, "utf8");
      console.log("Grammar JSON read successfully");
      
      // Save a copy to test_grammar.json for the parser
      fs.writeFileSync(testGrammarJsonPath, grammarData);
      console.log("Grammar JSON copied to test_grammar.json");
      
      // Also copy to build directory if it exists
      if (fs.existsSync(buildDirPath)) {
        fs.writeFileSync(buildTestGrammarPath, grammarData);
        console.log("Grammar JSON copied to build/test_grammar.json");
      }
      
      res.json(JSON.parse(grammarData));
    } catch (err) {
      console.error("Could not read grammar.json:", err);
      res.status(500).json({ error: "Could not read grammar output." });
    }
  });
});

//ROUTE first follow
app.get("/first-follow", (req, res) => {
  const backendPath = path.join(__dirname, '..', 'backend');
  const exePath = path.join(backendPath, 'first_follow_run.exe');
  const filePath = path.join(backendPath, 'first_follow.json');
  const buildDirPath = path.join(backendPath, 'build');

  exec(`"${exePath}"`, { cwd: backendPath }, (error, stdout, stderr) => {
    if (error) {
      console.error("error:", stderr);
      return res.status(500).send("Failed to generate first follow.");
    }

    // Read and send the resulting first_follow.json
    try {
      const firstFollowData = fs.readFileSync(filePath, "utf8");
      
      // If the build directory exists, also save a copy there
      if (fs.existsSync(buildDirPath)) {
        fs.writeFileSync(path.join(buildDirPath, 'first_follow.json'), firstFollowData);
        console.log("First/Follow data copied to build directory");
      }
      
      res.json(JSON.parse(firstFollowData));
    } catch (err) {
      console.error("Could not read first_follow.json");
      res.status(500).send("Could not read output.");
    }
  });
});

//parsing table route
app.get("/parsing-table", (req, res) => {
  const backendPath = path.join(__dirname, '..', 'backend');
  const exePath = path.join(backendPath, 'parsing_table_run.exe');
  const jsonPath = path.join(backendPath, 'parsing_table.json');
  const testJsonPath = path.join(backendPath, 'test_parsing_table.json');
  const buildDirPath = path.join(backendPath, 'build');

  exec(`"${exePath}"`, { cwd: backendPath }, (error, stdout, stderr) => {
    if (error) {
      console.error("Parsing table generation error:", stderr);
      return res.status(500).send("Failed to generate parsing table.");
    }

    // Read the parsing_table.json
    fs.readFile(jsonPath, "utf8", (err, data) => {
      if (err) {
        console.error("Error reading parsing_table.json:", err);
        return res.status(500).send("Could not read parsing table output.");
      }

      try {
        // Parse the original parsing table
        const originalTable = JSON.parse(data);
        
        // Copy to test_parsing_table.json for the parser - maintain original format
        fs.writeFileSync(testJsonPath, data);
        console.log("Parsing table copied to test_parsing_table.json");
        
        // Also copy to build directory if it exists
        if (fs.existsSync(buildDirPath)) {
          fs.writeFileSync(path.join(buildDirPath, 'test_parsing_table.json'), data);
          console.log("Parsing table copied to build/test_parsing_table.json");
        }
        
        // Return the original format to the frontend
        res.json(originalTable);
      } catch (parseErr) {
        console.error("Invalid JSON format in parsing_table.json:", parseErr);
        res.status(500).send("Parsing table data is not valid JSON.");
      }
    });
  });
});

// LL(1) Parser route
app.post("/run-parser", async (req, res) => {
  try {
    // First ensure the parser is built
    await buildParser();
    
    const backendPath = path.join(__dirname, '..', 'backend');
    const buildPath = path.join(backendPath, 'build');
    const exePath = path.join(buildPath, 'parser_run.exe');
    const inputPath = path.join(buildPath, 'test_input_tokens.json');
    const grammarPath = path.join(buildPath, 'test_grammar.json');
    const parsingTablePath = path.join(buildPath, 'test_parsing_table.json');
    const tracePath = path.join(buildPath, 'parse_trace.json');
    
    console.log("Parser executable path:", exePath);
    
    // Ensure build directory exists
    if (!fs.existsSync(buildPath)) {
      fs.mkdirSync(buildPath, { recursive: true });
    }
    
    // Delete any existing parse trace file to ensure we don't use old data
    if (fs.existsSync(tracePath)) {
      try {
        fs.unlinkSync(tracePath);
        console.log("Removed old parse trace file");
      } catch (err) {
        console.error("Failed to remove old parse trace:", err);
      }
    }
    
    // Check if all required files exist
    if (!fs.existsSync(exePath)) {
      return res.status(500).json({ error: "Parser executable not found. Please build the parser first." });
    }
    
    // Get input tokens from request
    const inputTokens = req.body.tokens;
    
    // Prepare tokens in the format expected by the parser
    let processedTokens = [];
    
    if (inputTokens && inputTokens.length > 0) {
      // Convert frontend tokens to the format expected by the parser
      processedTokens = inputTokens.map(token => {
        // Extract just the token type without any description
        let tokenType = token.type;
        if (tokenType.includes(" ")) {
          tokenType = tokenType.split(" ")[0];
        }
        
        // Special handling for 'identifier' since we need to use 'id' in the parser
        if (tokenType === "identifier") {
          tokenType = "id";
        }
        
        return {
          type: tokenType,
          value: token.value,
          line: token.line,
          column: 1 // Default column if not provided
        };
      });
    } else {
      // Use default tokens for testing if none provided
      processedTokens = [
        { type: "id", value: "x", line: 1, column: 1 },
        { type: "+", value: "+", line: 1, column: 3 },
        { type: "id", value: "y", line: 1, column: 5 },
        { type: "*", value: "*", line: 1, column: 7 },
        { type: "id", value: "z", line: 1, column: 9 }
      ];
    }
    
    console.log("Using tokens:", JSON.stringify(processedTokens, null, 2));
    
    // Write tokens to file
    fs.writeFileSync(inputPath, JSON.stringify(processedTokens, null, 2));
    console.log("Tokens written to:", inputPath);
    
    // Run the parser from the build directory
    console.log("Running parser:", `"${exePath}"`);
    
    const parserCmd = `"${exePath}" "${inputPath}" "${grammarPath}" "${parsingTablePath}" "${tracePath}"`;
    console.log("Full parser command:", parserCmd);
    
    exec(parserCmd, { cwd: buildPath }, (error, stdout, stderr) => {
        console.log("Parser stdout:", stdout);
        if (stderr) console.log("Parser stderr:", stderr);
        
        // Check if trace file exists
        if (!fs.existsSync(tracePath)) {
            console.error("Parser did not generate trace file");
            return res.status(500).json({ 
                error: "Parser failed to generate trace file",
                stdout: stdout,
                stderr: stderr
            });
        }
        
        try {
            // Read parse trace
            const traceData = fs.readFileSync(tracePath, 'utf8');
            let traceJson = JSON.parse(traceData);
            
            // If parsing failed but we have trace data, still try to build a tree
            if (error) {
                console.error("Parser execution error:", error);
                
                // If we have trace data, try to use it
                if (traceJson && traceJson.length > 0) {
                    const treeRoot = generateParseTree(traceJson);
                    return res.json({
                        success: false,
                        message: "Parsing failed but partial trace available",
                        error: stderr || stdout || "Parser execution failed",
                        parseTrace: traceJson,
                        parseTree: treeRoot
                    });
                } else {
                    return res.status(500).json({
                        error: "Parser failed with no usable trace",
                        stdout: stdout,
                        stderr: stderr
                    });
                }
            }
            
            // Generate tree structure from trace for successful parse
            const treeRoot = generateParseTree(traceJson);
            
            // Return both trace and tree data
            res.json({
                success: true,
                parseTrace: traceJson,
                parseTree: treeRoot
            });
        } catch (err) {
            console.error("Error processing parser output:", err);
            res.status(500).json({ 
                error: "Failed to process parser output: " + err.message,
                stdout: stdout,
                stderr: stderr
            });
        }
    });
  } catch (err) {
    console.error("Error in run-parser route:", err);
    res.status(500).json({ error: err.message });
  }
});

// Function to generate a parse tree from trace data
function generateParseTree(traceData) {
  // Create root node from first entry's stack_top
  const rootNode = {
    symbol: traceData[0].stack_top,
    children: []
  };
  
  // Stack to keep track of current node path
  let currentNode = rootNode;
  let nodeStack = [];
  
  for (const entry of traceData) {
    if (entry.action.startsWith('expand:')) {
      // Extract the production rule
      const production = entry.action.substring('expand: '.length);
      const parts = production.split(' → ');
      const lhs = parts[0];
      
      // Get the right-hand side symbols
      let rhs = [];
      if (parts.length > 1) {
        rhs = parts[1].split(' ').filter(symbol => 
          symbol !== 'ε' && symbol !== 'Îµ' // Skip epsilon symbols
        );
      }
      
      // Create child nodes for each symbol in RHS
      for (const symbol of rhs) {
        const childNode = {
          symbol: symbol,
          children: []
        };
        currentNode.children.push(childNode);
      }
      
      // If there are children, move to the first child
      if (currentNode.children.length > 0) {
        nodeStack.push(currentNode);
        currentNode = currentNode.children[0];
      }
    } 
    else if (entry.action === 'match') {
      // When a terminal is matched, we need to move to the next node
      if (nodeStack.length > 0) {
        const parent = nodeStack[nodeStack.length - 1];
        const childIndex = parent.children.indexOf(currentNode);
        
        if (childIndex < parent.children.length - 1) {
          // Move to next sibling
          currentNode = parent.children[childIndex + 1];
        } else {
          // Move back to parent
          currentNode = parent;
          nodeStack.pop();
        }
      }
    }
  }
  
  return rootNode;
}

// Start the server
const PORT = 3000;
app.listen(PORT, () => {
  console.log(`Server is running at http://localhost:${PORT}`);
});
