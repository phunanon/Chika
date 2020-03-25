String.prototype.rp = function(pad, len) { while (pad.length < len) { pad += pad; } return this + pad.substr(0, len-this.length); }
String.prototype.lp = function(pad, len) { while (pad.length < len) { pad += pad; } return pad.substr(0, len-this.length) + this; }
Array.prototype.remove = function (fn) { return this.filter(it => !fn(it)); }

//https://stackoverflow.com/a/24947000
function int2buf(a){return arr=new ArrayBuffer(4),view=new DataView(arr),view.setUint32(0,a,!1),arr}
//https://stackoverflow.com/a/40031979
function buf2hex(a){return Array.prototype.map.call(new Uint8Array(a),a=>("00"+a.toString(16)).slice(-2)).join("")}
const insert = (arr, index, newItem) => [...arr.slice(0, index), newItem, ...arr.slice(index)];
const flatten = (arr) => arr.reduce((flat, next) => flat.concat(Array.isArray(next) ? flatten(next) : next), []);
const numToHex = (n, bytes) => buf2hex(int2buf(n)).toUpperCase().slice(8 - bytes*2);
const numToLEHex = (n, bytes) => numToHex(n, bytes).match(/.{2}/g).reverse().join("");
const bytesToHex = nums => nums.map(n => numToHex(n, 1)).join("");
const last = arr => arr[arr.length - 1];
const isObject = o => o === Object(o);
const isString = s => typeof s === "string";
const isChikaNum = s => isString(s) && (s.search(/^0x[\dA-F]+$/i) != -1 || s.search(/^-?\d+[w|i]*$/) != -1);
const num = s => parseInt(s.match(/-?\d+/g).join(""));


const
  Form_Eval = 0x00,
  Form_If = 0x01, Form_Or = 0x02, Form_And = 0x03, Form_Case = 0x04,
  Val_True = 0x05, Val_False = 0x06, Val_Str = 0x07, Param_Val = 0x08,
  Bind_Mark = 0x09, Bind_Val = 0x0A, XBind_Val = 0x0B,
  Val_U08 = 0x10, Val_U16 = 0x11, Val_I32 = 0x12, Val_Char = 0x13,
  Val_Args = 0x19, Var_Op = 0x1A, Var_Func = 0x1B, Val_Nil = 0x1E,
  Op_Func = 0x22, Op_Var = 0x2A, Op_Param = 0x2B;
const strOps =
  {"if":     0x23, "or":     0x24, "and":    0x25, "case":  0x26,
   "not":    0x27, "return": 0x2E, "recur":  0x2F,
   "=":      0x30, "==":     0x31, "!=":     0x32, "!==":    0x33,
   "<":      0x34, "<=":     0x35, ">":      0x36, ">=":     0x37,
   "+":      0x38, "-":      0x39, "*":      0x3A, "/":      0x3B, "mod":    0x3C, "pow": 0x3D,
   "~":      0x40, "&":      0x41, "|":      0x42, "^":      0x43, "<<":     0x44, ">>":  0x45,
   "p-mode": 0x60, "dig-r":  0x61, "dig-w":  0x62, "ana-r":  0x63, "ana-w":  0x64,
   "file-r": 0x6A, "file-w": 0x6B, "file-a": 0x6C, "file-d": 0x6D,
   "str":    0xA0, "type":   0xAA, "cast":   0xAB,
   "vec":    0xB0, "nth":    0xB1, "len":    0xB2, "sect":   0xB3, "b-sect": 0xB4,
   "..":     0xBA, "reduce": 0xBB, "map":    0xBC, "for":    0xBD, "loop":   0xBE,
   "binds":  0xCC, "val":    0xCD, "do":     0xCE,
   "pub":    0xD0, "sub":    0xD1, "unsub":  0xD2,
   "ms-now": 0xE0, "sleep":  0xE1,
   "print":  0xEE, "debug":  0xEF, "load":   0xF0, "halt":   0xFF};
const literals =
  {"N": Val_Nil, "T": Val_True, "F": Val_False,
   "nl": '\n', "sp": ' '};
const formCodes =
  {"if": Form_If, "or": Form_Or, "and": Form_And, "case": Form_Case};


const walkItems = (arr, pred, func) =>
  arr.map(i => Array.isArray(i) ? walkItems(i, pred, func) : (pred(i) ? func(i) : i));

const walkArrays = (arr, pred, func) =>
  Array.isArray(arr)
    ? arr.map(i => Array.isArray(i) ? walkArrays((pred(i) ? func(i) : i), pred, func) : i)
    : arr;

function extractStrings (source) {
  const strings = [];
  let numStr = 0;
  source = source.replace(/\\"/g, "/quotes");
  source = source.replace(/"(.*?)"/g, m => { strings.push(m.slice(1,-1)); return "`"+(numStr++)+"`"; });
  source = source.replace(/\/quotes/g, "\\\"");
  return {source, strings};
}

function replaceCharacters (source) {
  return source.replace(/(\\(\w{2}|.))/g,
    c => "`c" + (c.length == 2 ? c[1] : literals[c.slice(1)]).charCodeAt(0) + "`");
}

const formise = s =>
  eval(
    "["+
    s.replace(/\s*\n\s*/g, " ")
     .trim()
     .replace(/ /g,    "', '")
     .replace(/\(/g,   "', ['")
     .replace(/{/g,    "', ['*fn*', '")
     .replace(/[)}]/g, "'], '")
     .replace(/(^', |, '$|, '')/g, "")
    +"]");

function funcise (forms) {
  const isFunc = f => f[0].startsWith("fn");
  let   funcs = forms.filter(isFunc);
  funcs.unshift(forms.filter(f => !isFunc(f)));
  //Give entry function a source head
  funcs[0] = ["fn", "entry", "params"].concat(funcs[0]);
  //Either include a halting heartbeat or insert the heartbeat to the 2nd
  if (!funcs.some(f => f[1] == "heartbeat"))
    funcs.splice(1, 0, ["fn", "heartbeat", ["halt"]]);
  else
    funcs.splice(1, 0, funcs.splice(funcs.findIndex(f => f[1] == "heartbeat"), 1)[0]);
  //Include function ID's
  funcs.forEach((f, i) => f.unshift({n: i, b: 2, info: `func ID: ${f[1]}`}));
  return funcs;
}

function compile (source, ramRequest) {

  const startTime = new Date();

  //Extract all strings, and replace them with "`n`"
  const extractedStrings = extractStrings(source);
  const strings = extractedStrings.strings;
  source = extractedStrings.source;

  //Replace all characters, and replace them with "`c[charcode]`"
  source = replaceCharacters(source);
  
  //Source-specific modifications, some applicable only to this implementation
  source = source.replace(/\/\*[\s\S]+\*\//g, "") //Multiline comments
                 .replace(/\/\/.+\n/g, "\n")      //Single line comments
                 .replace(/\/\/.+$/g, "")         //Document-final comment
                 .replace(/;/g, "")               //Semicolon whitespace
                 .replace(/,\s*/g, "")            //Comma whitespace
                 .replace(/#(?!\d)/g, "#0");      //# -> #0
  
  //Replace all vectors with (vec ...) form
  source = source.replace(/\[/g, "(vec ").replace(/\]/g, ")");

  //Make forms into nested arrays
  let forms = formise(source);

  //Collect inline functions
  let inlineFuncs = [];
  forms = walkArrays(forms, a => a[0] == "*fn*",
    a => {
      fn = "fn" + inlineFuncs.length;
      const body = a.slice(1);
      let nParam = 0;
      walkItems(a, s => s.startsWith("#"), s => nParam = Math.max(num(s), nParam));
      a = ["fn", fn].concat([...Array(nParam + 1).keys()].map(n => `#${n}`)).concat([body]);
      inlineFuncs.push(a);
      return fn;
    });
  forms = forms.concat(inlineFuncs);
  //Collect entry function forms, and user functions
  let funcs = funcise(forms);

  //Create function register
  const funcRegister = funcs.map((f, i) => ({sym: f[2], paras: f.filter(isString).slice(2)}));
  //Remove func signature
  funcs = funcs.map(f => f.remove(isString));

  //Operator to the tail position, as object to insulate from func/var/param detection
  funcs = walkArrays(funcs, a => isString(a[0]), a => a.slice(1).concat([{op: a[0]}]));

  //Prepend forms with correct form code
  function appendFormCode (form) {
    const op = last(form);
    const formCode = formCodes[op.op];
    const fCode = formCode ? formCode : Form_Eval;
    return [{n: fCode, b: 1, info: `form: ${op.op}`}].concat(form);
  }
  funcs = funcs.map(f => walkArrays(f, a => a, appendFormCode));

  //Serialise integers
  function serialiseNum (s) {
    let bWidth = s.endsWith("i") ? 4 : (s.endsWith("w") ? 2 : 1);
    if (bWidth != 1) s.slice(0, -1);
    if (s.startsWith("0x"))
      bWidth = {3:1,4:1,5:2,6:2,7:4,8:4,9:4,10:4}[s.length];
    const type = {1:Val_U08, 2:Val_U16, 4:Val_I32}[bWidth];
    return {hex: numToHex(type, 1)
                 + numToLEHex(parseInt(s), bWidth),
            info: `int: ${s}`};
  }
  funcs = walkItems(funcs, isChikaNum, serialiseNum);

  //Serialise strings back in, and characters
  function serialiseStrChar (s) {
    const isChar = s[1] == 'c';
    let hex = "";
    hex += numToHex(isChar ? Val_Char : Val_Str, 1);
    hex += isChar
      ? numToHex(num(s), 1)
      : Array.from(strings[num(s)])
             .map(c => numToHex(c.charCodeAt(0), 1))
             .join("")
             + "00";
    return {hex: hex, info: (isChar ? "char" : "string")};
  }
  funcs = walkItems(funcs, i => i[0] == "`", serialiseStrChar);

  //Replace symbol literals
  function replaceLiteral (l) {
    return {n: literals[l], b: 1, info: `literal: ${l}`};
  }
  funcs = walkItems(funcs, i => literals[i] != undefined, replaceLiteral);

  //Replace parameters, bind marks, and bind references
  const binds = [];
  const bindsDefined = [];
  function argOrVarToHex (sym, fi) {
    if (sym == "args")
      return {hex: numToHex(Val_Args, 1), info: `args val`};
    //Check if function parameter
    const param = funcRegister[fi].paras.indexOf(sym);
    if (param != -1)
      return {hex: bytesToHex([Param_Val, param]), info: `param: ${sym}`};
    //Check between variable reference or binding
    let isBindMark = sym.endsWith("=") && !["!=", "="].includes(sym);
    if (isBindMark) sym = sym.slice(0, -1);
    //Check if op, function, or variable
    let variHex;
    let op = strOps.hasOwnProperty(sym);
    let func = funcRegister.findIndex(f => f.sym == sym);
    //If op
    if (op) {
      variHex = numToHex(Var_Op, 1)
                + numToLEHex(strOps[sym], 1);
    } else
    //If func
    if (func != -1) {
      variHex = numToHex(Var_Func, 1)
                + numToLEHex(func, 2);
    } else
    //If not op or func, meaning binding or binding reference
    {
      let isXBind = sym.startsWith(".");
      if (isXBind) sym = sym.slice(1);
      if (!binds.includes(sym)) {
        binds.push(sym);
        bindsDefined.push(false);
      }
      if (isBindMark)
        bindsDefined[binds.indexOf(sym)] = true;
      const type = isBindMark ? Bind_Mark : (isXBind ? XBind_Val : Bind_Val);
      const bIdx = binds.indexOf(sym);
      variHex = numToHex(type, 1) + numToLEHex(bIdx, 2);
    }
    return {hex: variHex, info: `${isBindMark ? "bind" : "var"}: ${sym}`};
  }
  funcs = funcs.map((f, fi) => walkItems(f, isString, arg => argOrVarToHex(arg, fi)));
  bindsDefined.forEach((u, i) => !u ? console.log(`WARN: ${binds[i]} undefined`) : "");

  //Replace tail-positioned native and program functions
  function funcHex (sym, fi) {
    let funcIndex = funcRegister.findIndex(f => f.sym == sym);
    //If this is a user function
    if (funcIndex != -1)
      return {hex: numToHex(Op_Func, 1)
                   + numToLEHex(funcIndex, 2),
              info: `prog fn: ${sym}`}
    else
    //If this is an param or variable
    {
      //If param
      let vArg = funcRegister[fi].paras.indexOf(sym)
      if (vArg != -1) {
        return {hex: bytesToHex([Op_Param, vArg]),
                info: `param op/fn: ${sym}`}
      } else
      //It's a variable
      {
        let vIndex = binds.indexOf(sym);
        if (vIndex != -1)
          return {hex: numToHex(Op_Var, 1)
                      + numToLEHex(vIndex, 2),
                  info: `var op/fn: ${sym}`}
        else {
          console.error(`Func ${sym} not found`);
          return {hex: "", info: `err: func ${sym} not found`};
        }
      }
    }
  }
  const opOrFuncHex = (op, fi) =>
    strOps[op.op] == undefined
    ? funcHex(op.op, fi)
    : {n: strOps[op.op], b: 1, info: `op: ${op.op}`};
  funcs = funcs.map((f, fi) =>
            walkArrays(f, a => last(a).op != undefined,
              a => a.slice(0, -1).concat(opOrFuncHex(last(a), fi))));

  //Prepend function length
  const itemsLen = items => items.reduce((acc, i) => acc + (i.hex ? i.hex.length / 2 : i.b), 0);
  funcs = funcs.map(f => {
    f = flatten(f);
    return insert(f, 1, {n: itemsLen(f.slice(1)), b: 2, info: "func len"});
  });

  //console.log(funcs);

  //assembly to image
  const image =
    numToLEHex(ramRequest, 4) + //Prepend program RAM request
    flatten(funcs)
      .filter(isObject)
      .map(n => n.hex == undefined ? numToLEHex(n.n, n.b) : n.hex)
      .join("");

  console.log(`${(image.length / 2)} B in ${((new Date()) - startTime)}ms`);
  return {assembly: JSON.stringify(funcs, null, ' '), image};
}

module.exports = function() { 
    this.compile = compile;
}
