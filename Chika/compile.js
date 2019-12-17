String.prototype.rp = function(pad, len) { while (pad.length < len) { pad += pad; } return this + pad.substr(0, len-this.length); }
String.prototype.lp = function(pad, len) { while (pad.length < len) { pad += pad; } return pad.substr(0, len-this.length) + this; }
Array.prototype.remove = function (fn) { return this.filter(it => !fn(it)); }

const insert = (arr, index, newItem) => [...arr.slice(0, index), newItem, ...arr.slice(index)];
const flatten = (arr) => arr.reduce((flat, next) => flat.concat(Array.isArray(next) ? flatten(next) : next), []);
const numToHex = (n, bytes) => n.toString(16).toUpperCase().lp("0", bytes*2);
const numToLEHex = (n, bytes) => numToHex(n, bytes).match(/.{2}/g).reverse().join("");
const bytesToHex = nums => nums.map(n => numToHex(n, 1)).join("");
const last = arr => arr[arr.length - 1];
const isObject = o => o === Object(o);
const isString = s => typeof s === "string";
const isChikaNum = s => isString(s) && s.search(/\d+[W|I]*/) != -1 && s.match(/\d+[W|I]*/g).join("") == s;
const num = s => parseInt(s.match(/\d+/g).join(""));


const
  Form_Eval = 0x00, Form_If = 0x01, Form_Or = 0x02, Form_And = 0x03,
  Val_True = 0x04, Val_False = 0x05, STR = 0x06, Eval_Arg = 0x07,
  Bind_Var = 0x08, Eval_Var = 0x09,
  Val_U08 = 0x10, Val_U16 = 0x11, Val_I32 = 0x12, NIL = 0x21, FNC = 0x22;
const strFuncs =
  {"if": 0x23, "or": 0x24, "and": 0x25, "=": 0x31, "==": 0x32,
   "+": 0x33, "str": 0x44, "vec": 0xBB, "nth": 0xCC, "val": 0xCD,
   "do": 0xCE, "print": 0xEE};
const literals =
  {"nil": NIL, "true": Val_True, "false": Val_False};
const formCodes =
  {"if": Form_If, "or": Form_Or, "and": Form_And};


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
  funcs.forEach((f, i) => f.unshift({n: i, b: 2, info: `func ID: ${f[1]}`}));
  return funcs;
}

function compile (source) {

  //Extract all strings, and replace them with "`n`"
  const extractedStrings = extractStrings(source);
  const strings = extractedStrings.strings;
  source = extractedStrings.source;
  
  //Remove all comments and commas
  source = source.replace(/;.+\n/g, "\n").replace(/,/g, "");
  
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

  //Prepend forms with correct form code
  function appendFormCode (form) {
    const op = last(form);
    const formCode = formCodes[op];
    const fCode = formCode != undefined ? formCode : Form_Eval;
    return [{n: fCode, b: 1, info: `form: ${op}`}].concat(form);
  }
  funcs = funcs.map(f => walkArrays(f, a => a, appendFormCode));

  //Serialise integers
  function serialiseNum (s) {
    const isU16 = s.endsWith("W");
    const isI32 = s.endsWith("I");
    if (isU16 || isI32)
      s = s.slice(0, -1);
    const type = isI32 ? Val_I32 : (isU16 ? Val_U16 : Val_U08);
    const len = isI32 ? 4 : (isU16 ? 2 : 1);
    return {hex: numToHex(type, 1)
                 + numToLEHex(parseInt(s), len),
            info: `int: ${s}`};
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
    ? {hex: numToHex(FNC, 1)
            + numToLEHex(funcRegister.findIndex(f => f.sym == sym), 2),
       info: `prog fn: ${sym}`}
    : {n: strFuncs[sym], b: 1, info: `op: ${sym}`};
  funcs = walkArrays(funcs, a => isString(last(a)), a => a.slice(0, -1).concat(funcHex(last(a))));

  //Replace symbol literals
  function replaceLiteral (l) {
    return {n: literals[l], b: 1, info: `literal: ${l}`};
  }
  funcs = walkItems(funcs, i => literals[i] != undefined, replaceLiteral);

  //Replace arguments, variables, and binds
  const variables = [];
  function argOrVarToHex (sym, fi) {
    const param = funcRegister[fi].paras.indexOf(sym);
    if (param != -1)
      return {hex: bytesToHex([Eval_Arg, param]), info: `arg: ${sym}`};
    //Check if variable or bind
    let bind = sym.endsWith("=");
    if (bind) sym = sym.slice(0, -1);
    let vari = variables.indexOf(sym);
    if (vari == -1)
      variables.push(sym);
    const variHex = numToHex(bind ? Bind_Var : Eval_Var, 1)
                    + numToLEHex(variables.indexOf(sym), 2);
    return {hex: variHex, info: `${bind ? "bind" : "var"}: ${sym}`};
  }
  funcs = funcs.map((f, fi) => walkItems(f, isString, arg => argOrVarToHex(arg, fi)));

  //Prepend function length
  const itemsLen = items => items.reduce((acc, i) => acc + (i.hex ? i.hex.length / 2 : i.b), 0);
  funcs = funcs.map(f => {
    f = flatten(f);
    return insert(f, 1, {n: itemsLen(f.slice(1)), b: 2, info: "func len"});
  });

  //assembly to image
  const image =
    flatten(funcs)
      .filter(isObject)
      .map(n => n.hex == undefined ? numToLEHex(n.n, n.b) : n.hex)
      .join("");

  return {assembly: JSON.stringify(funcs, null, ' '), image};
}

module.exports = function() { 
    this.compile = compile;
}
