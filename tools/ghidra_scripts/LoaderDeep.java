// Decompile the .v2p param-read sequence + stream primitives.
// Args: outPath, then tokens: "ref:0xADDR" (decompile callers) or "fn:0xADDR" (decompile function containing).
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.LinkedHashSet;

public class LoaderDeep extends GhidraScript {
    DecompInterface di;
    PrintWriter pw;
    LinkedHashSet<Long> done = new LinkedHashSet<>();

    public void run() throws Exception {
        String[] a = getScriptArgs();
        di = new DecompInterface();
        di.openProgram(currentProgram);
        pw = new PrintWriter(new FileWriter(a[0]));
        ReferenceManager rm = currentProgram.getReferenceManager();
        for (int k = 1; k < a.length; k++) {
            String tok = a[k];
            if (tok.startsWith("ref:")) {
                long va = Long.decode(tok.substring(4));
                pw.println("// ==== CALLERS of 0x" + Long.toHexString(va) + " ====");
                for (Reference r : rm.getReferencesTo(toAddr(va))) {
                    Function f = getFunctionContaining(r.getFromAddress());
                    pw.println("//   ref @0x" + Long.toHexString(r.getFromAddress().getOffset()) +
                               (f == null ? " (none)" : " in " + f.getName()));
                    if (f != null) emit(f);
                }
            } else if (tok.startsWith("range:")) {
                String[] r = tok.substring(6).split("-");
                long lo = Long.decode(r[0]), hi = Long.decode(r[1]);
                pw.println("// ==== RANGE 0x" + Long.toHexString(lo) + "-0x" + Long.toHexString(hi) + " ====");
                ghidra.program.model.listing.FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
                while (it.hasNext()) {
                    Function f = it.next();
                    long ea = f.getEntryPoint().getOffset();
                    if (ea >= lo && ea < hi) emit(f);
                }
            } else {
                long va = Long.decode(tok.startsWith("fn:") ? tok.substring(3) : tok);
                Function f = getFunctionContaining(toAddr(va));
                if (f == null) { pw.println("// no func at 0x" + Long.toHexString(va)); continue; }
                emit(f);
            }
        }
        pw.close();
        println("LoaderDeep: wrote " + done.size() + " funcs");
    }

    void emit(Function f) {
        long ea = f.getEntryPoint().getOffset();
        if (!done.add(ea)) return;
        DecompileResults res = di.decompileFunction(f, 120, monitor);
        pw.println("// ===== " + f.getName() + " @ 0x" + Long.toHexString(ea) + " =====");
        if (res != null && res.decompileCompleted()) pw.println(res.getDecompiledFunction().getC());
        else pw.println("// decompile failed: " + (res == null ? "null" : res.getErrorMessage()));
        pw.println();
    }
}
