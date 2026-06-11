// Force-decompile the function containing each given address (scan back for a 55 8B EC prologue
// and create a function if Ghidra didn't). Args: outPath then hex addresses. Headless post-script.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;
import java.io.FileWriter;
import java.io.PrintWriter;

public class DecompForce extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter pw = new PrintWriter(new FileWriter(a[0]));
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        Memory mem = currentProgram.getMemory();
        for (int k = 1; k < a.length; k++) {
            long va = Long.decode(a[k]);
            Address addr = toAddr(va);
            Function f = getFunctionContaining(addr);
            if (f == null) {
                Address s = addr;
                for (int off = 0; off < 0x800; off++) {
                    Address c = addr.subtract(off);
                    int b0 = mem.getByte(c) & 0xff;
                    int b1 = mem.getByte(c.add(1)) & 0xff;
                    int b2 = mem.getByte(c.add(2)) & 0xff;
                    if (b0 == 0x55 && b1 == 0x8b && b2 == 0xec) { s = c; break; }
                }
                try { disassemble(s); f = createFunction(s, null); } catch (Exception e) {}
                if (f == null) f = getFunctionContaining(s);
            }
            pw.println("// ==== forced @0x" + Long.toHexString(va) + " -> " +
                       (f == null ? "FAIL" : f.getName() + " @0x" + Long.toHexString(f.getEntryPoint().getOffset())) + " ====");
            if (f != null) {
                DecompileResults r = di.decompileFunction(f, 120, monitor);
                if (r != null && r.decompileCompleted()) pw.println(r.getDecompiledFunction().getC());
                else pw.println("// decompile failed");
            }
            pw.println();
        }
        pw.close();
        println("DecompForce done");
    }
}
