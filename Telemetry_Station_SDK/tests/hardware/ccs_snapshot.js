/* Read GPIO only from an already halted, user-created modern CCS session.
 * Never connect, flash, reset, halt, resume or write target memory here. */
const bases = {1:0x20,2:0x28,3:0x18,4:0x1c,5:0x30,6:0x34};
function capture(session, identity) {
    for (const k of ['board_serial','board_revision','phase','image_sha256','pinmap_sha256']) {
        if (!identity || typeof identity[k] !== 'string' || !identity[k].trim()) throw Error('Missing '+k);
    }
    for (const k of ['image_sha256','pinmap_sha256']) {
        if (!/^[a-f0-9]{64}$/.test(identity[k])) throw Error('Invalid '+k);
    }
    if (!session.target.isHalted()) throw Error('Use an approved safe halted checkpoint');
    const registers = {};
    for (let port=1;port<=6;port++) {
        for (const [suffix,offset] of Object.entries({IN:0,OUT:1,DIR:2,SEL:port<=2?6:3})) {
            const value=Number(session.memory.readOne(BigInt(bases[port]+offset),8));
            if (!Number.isInteger(value) || value<0 || value>255) throw Error('Invalid register read');
            registers[`P${port}${suffix}`]=value;
        }
    }
    return {schema:1,kind:'gpio_snapshot',...identity,registers,captured_utc:new Date().toISOString(),
        scope:'Register observation only. Image hash is operator supplied, not read-back verification.'};
}
module.exports={capture};
