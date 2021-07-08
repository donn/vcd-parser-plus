const is_node = typeof process === 'object' && typeof process.versions === 'object' && typeof process.versions.node === 'string';
if (is_node) {
    var Module = require("./build/libvcdparse");
}

function Helper(Module) {
    this.Module = Module;

    this.to_cstr = (str) => {
        let bytes = Module.lengthBytesUTF8(str);
        let c_str = Module._malloc(bytes + 1);
        Module.stringToUTF8(str, c_str, bytes + 1);
        return {c_str, free: ()=> Module._free(c_str)};
    }

    this.from_cstr = (c_str, free=false) => {
        let str = Module.UTF8ToString(c_str);
        if (free) {
            Module._free(c_str);
        }
        return str;
    }

    return this;
}

async function parse(input) {
    await new Promise(r=> Module.onRuntimeInitialized = r);

    let helper = Helper(Module);

    let ci = helper.to_cstr(input);

    let rawString = Module._VCDToJSON(ci.c_str);
    ci.free()

    let json = helper.from_cstr(rawString, true);
    return JSON.parse(json);
}

if (is_node) {
    if (require.main === module) {
        const fs = require("fs");

        async function main() {
            let args = process.argv.slice(2);
            if (args.length != 1) {
                console.error(`Usage: ${process.argv.slice(0, 2).join(" ")} <file>`);
                process.exit(64);
            }
            let input = fs.readFileSync(args[0], "utf8");
            console.log(JSON.stringify(await parse(input), null, 2));
        }
        main();
    }
    module.exports = parse;

} else {
    window.parse = parse;
}
