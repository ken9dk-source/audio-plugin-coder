// Force-create a function EXACTLY at each given address (vtable entry = method entry point) and decompile.
// Args: outPath then hex addresses. Headless post-script. Run against an already-analyzed (kept) project.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.FileWriter;
import java.io.PrintWriter;

public class DecompExact extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter pw = new PrintWriter(new FileWriter(a[0]));
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        for (int k = 1; k < a.length; k++) {
            long va = Long.decode(a[k]);
            Address addr = toAddr(va);
            Function f = getFunctionAt(addr);
            if (f == null) {
                try { disassemble(addr); } catch (Exception e) {}
                try { f = createFunction(addr, null); } catch (Exception e) {}
                if (f == null) f = getFunctionContaining(addr);
            }
            pw.println("// ==== @0x" + Long.toHexString(va) + " -> " +
                       (f == null ? "FAIL" : f.getName() + " @0x" + Long.toHexString(f.getEntryPoint().getOffset())) + " ====");
            if (f != null) {
                DecompileResults r = di.decompileFunction(f, 120, monitor);
                if (r != null && r.decompileCompleted()) pw.println(r.getDecompiledFunction().getC());
                else pw.println("// decompile failed");
            }
            pw.println();
        }
        pw.close();
        println("DecompExact wrote to " + a[0]);
    }
}
