const lib = require("./build/libvcdparse");
const fs = require("fs");

function Helper(lib) {
    this.lib = lib;

    this.to_cstr = (str) => {
        let bytes = lib.lengthBytesUTF8(str);
        let c_str = lib._malloc(bytes + 1);
        lib.stringToUTF8(str, c_str, bytes + 1);
        return {c_str, free: ()=> lib._free(c_str)};
    }

    this.from_cstr = (c_str, free=false) => {
        let str = lib.UTF8ToString(c_str);
        if (free) {
            lib._free(c_str);
        }
        return str;
    }

    return this;
}

async function parse(input) {
    await new Promise(r=> lib.onRuntimeInitialized = r);

    let helper = Helper(lib);

    let ci = helper.to_cstr(input);

    let rawString = lib._VCDToJSON(ci.c_str);
    ci.free()

    let json = helper.from_cstr(rawString, true);
    return JSON.parse(json);
}

if (require.main === module) {
    async function main() {
        let input = fs.readFileSync("/donn/Wavedash/test/test.vcd", "utf8");
        console.log(JSON.stringify(await parse(input), null, 2));
    }
    main();
}

module.exports = parse;