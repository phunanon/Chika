String.prototype.rp = function(pad, len) { while (pad.length < len) { pad += pad; } return this + pad.substr(0, len-this.length); }
String.prototype.lp = function(pad, len) { while (pad.length < len) { pad += pad; } return pad.substr(0, len-this.length) + this; }
Array.prototype.remove = function (fn) { return this.filter(it => !fn(it)); }

const insert = (arr, index, newItem) => [...arr.slice(0, index), newItem, ...arr.slice(index)];
const numToHex = (n, bytes) => n.toString(16).toUpperCase().lp("0", bytes*2);
const numToLEHex = (n, bytes) => numToHex(n, bytes).match(/.{2}/g).reverse().join("");
const bytesToHex = nums => nums.map(n => numToHex(n, 1)).join("");
const last = arr => arr[arr.length - 1];
const isObject = o => o === Object(o);
const isString = s => typeof s === "string";
const isChikaNum = s => isString(s) && s.search(/\d+[W|B]*/) != -1 && s.match(/\d+[W|B]*/g).join("") == s;
const num = s => parseInt(s.match(/\d+/g).join(""));


const FRM = 0x00, STR = 0x01, ARG = 0x06, U08 = 0x10, U16 = 0x11, I32 = 0x12, FNC = 0x22;
const strFuncs =
  {"+": 0x33, "str": 0x44, "vec": 0xBB, "nth": 0xCC, "val": 0xCD, "print": 0xEE};


const walkItems = (arr, pred, func) =>
  arr.map(i => Array.isArray(i) ? walkItems(i, pred, func) : (pred(i) ? func(i) : i));

const walkArrays = (arr, pred, func) =>
  arr.map(i => Array.isArray(i) ? walkArrays((pred(i) ? func(i) : i), pred, func) : i);

function extractStrings (source) {
  const strings = [];
  let numStr = 0;
  source = source.replace(/"(.*?)"/g, m => { strings.push(m.slice(1,-1)); return "`"+(numStr++)+"`"; });
  return {source, strings};
}

const formise = s =>
  eval(
    "["+
    s.replace(/\s*\n\s*/g, "")
     .replace(/ /g, "\", \"")
     .replace(/\(/g, "\", [\"")
     .replace(/\)/g, "\"], \"")
     .replace(/(^\", |, \"$|, \"\")/g, "")
    +"]");

function funcise (forms) {
  const isFunc = f => f[0].startsWith("fn");
  let   funcs = forms.filter(isFunc);
  funcs.unshift(forms.filter(f => !isFunc(f)));
  //Give entry function a source head
  funcs[0] = ["fn", "entry", "params"].concat(funcs[0]);
  //Include function ID's
  funcs.forEach((f, i) => f.unshift({n: i, b: 2, info: "func ID"}));
  return funcs;
}

function compile (source) {

  //Extract all strings, and replace them with "`n`"
  const extractedStrings = extractStrings(source);
  const strings = extractedStrings.strings;
  source = extractedStrings.source;
  
  //Remove all comments
  source = source.replace(/;.+\n/g, "\n");
  
  //Replace all vectors with (vec ...) form
  source = source.replace(/\[/g, "(vec ").replace(/\]/g, ")");

  //Make forms into nested arrays
  const forms = formise(source);

  //Collect entry function forms, and user functions
  let funcs = funcise(forms);

  //Create function register
  const funcRegister = funcs.map((f, i) => ({sym: f[2], paras: f.filter(isString).slice(2)}));
  //Remove func signature
  funcs = funcs.map(f => f.remove(isString));

  //Operator to the tail position
  funcs = walkArrays(funcs, a => isString(a[0]), a => a.slice(1).concat([a[0]]));

  //Prepend forms with FRM
  funcs = funcs.map(f => walkArrays(f, a => a, a => [{n: FRM, b: 1, info: "form marker"}].concat(a)));

  //Serialise integers
  function serialiseNum (s) {
    const isWord = s.endsWith("W");
    const isByte = s.endsWith("B");
    if (isWord || isByte)
      s = s.slice(0, -1);
    const type = isWord ? U16 : (isByte ? U08 : I32);
    const len = isWord ? 2 : (isByte ? 1 : 4);
    return {hex: numToHex(type, 1)
                 + numToLEHex(parseInt(s), len),
            info: "int"};
  }
  funcs = walkItems(funcs, isChikaNum, serialiseNum);

  //Serialise strings back in
  const serialiseString = s =>
    ({hex: (numToHex(STR, 1)
            +Array.from(strings[num(s)])
                  .map(c => numToHex(c.charCodeAt(0), 1))
                  .join("")
            +"00"),
      info: "string"});
  funcs = walkItems(funcs, i => i[0] == "`", serialiseString);

  //Replace tail-positioned native and program functions
  const funcHex = sym =>
    strFuncs[sym] == undefined
    ? {hex: numToHex(FNC, 1) + numToLEHex(funcRegister.findIndex(f => f.sym == sym), 2), info: "prog func call"}
    : {n: strFuncs[sym], b: 1, info: "op call"};
  funcs = walkArrays(funcs, a => isString(last(a)), a => a.slice(0, -1).concat(funcHex(last(a))));

  //Replace arguments
  const argToHex = (arg, fi) => ({hex: bytesToHex([ARG, funcRegister[fi].paras.indexOf(arg)]), info: "arg"});
  funcs = funcs.map((f, fi) => walkItems(f, isString, arg => argToHex(arg, fi)));

  //Prepend function length
  const itemsLen = items => items.reduce((acc, i) => acc + (i.hex ? i.hex.length / 2 : i.b), 0);
  funcs = funcs.map(f => {
    f = f.flat(Infinity);
    return insert(f, 1, {n: itemsLen(f.slice(1)), b: 2, info: "func len"});
  });

  //assembly to image
  const image =
    funcs
      .flat(Infinity)
      .filter(isObject)
      .map(n => n.hex == undefined ? numToLEHex(n.n, n.b) : n.hex)
      .join("");

  return {assembly: JSON.stringify(funcs, null, ' '), image};
}

module.exports = function() { 
    this.compile = compile;
}
