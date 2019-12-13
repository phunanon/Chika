const fs = require("fs");
require('./compile.js')();

const inputFile  = process.argv[2];
const outputFile = inputFile.substr(0, inputFile.lastIndexOf(".")) + ".kua";

function compileFile (err, buf) {
  const compiled = compile(buf.toString());
  const image = Buffer.alloc(compiled.image.length / 2, compiled.image, "hex");
  fs.writeFile(outputFile, image, "binary",function(err) { });
}

fs.readFile(inputFile, compileFile);
