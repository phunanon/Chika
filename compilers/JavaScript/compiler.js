const fs = require("fs");
require('./compile.js')();

const inputFile  = process.argv[2];
const ramRequest = process.argv[3] != undefined ? parseInt(process.argv[3]) : 8192;
const outputFile = inputFile.substr(0, inputFile.lastIndexOf(".")) + ".kua";

function compileFile (err, buf) {
  process.stdout.write(`${inputFile}\t`);
  const compiled = compile(buf.toString(), ramRequest);
  const image = Buffer.alloc(compiled.image.length / 2, compiled.image, "hex");
  fs.writeFile(outputFile, image, "binary",function(err) { });
}

fs.readFile(inputFile, compileFile);
