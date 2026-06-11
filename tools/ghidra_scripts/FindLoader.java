// Find the .v2p chunk parser: (1) functions referencing given string addresses,
// (2) functions with a FourCC immediate operand (PRST/MSmp/VAZ /MID /STR /Samp).
// Args: outPath then hex string-addresses. Headless post-script.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.LinkedHashSet;
import java.util.HashSet;

public class FindLoader extends GhidraScript {
    DecompInterface di;
    PrintWriter pw;
    LinkedHashSet<Long> done = new LinkedHashSet<>();

    public void run() throws Exception {
        String[] a = getScriptArgs();
        String out = a[0];
        di = new DecompInterface();
        di.openProgram(currentProgram);
        pw = new PrintWriter(new FileWriter(out));
        ReferenceManager rm = currentProgram.getReferenceManager();

        // 1) references to the string addresses passed as args
        for (int k = 1; k < a.length; k++) {
            long va = Long.decode(a[k]);
            Address target = toAddr(va);
            pw.println("// ==== refs to 0x" + Long.toHexString(va) + " ====");
            for (Reference ref : rm.getReferencesTo(target)) {
                Address from = ref.getFromAddress();
                Function f = getFunctionContaining(from);
                pw.println("//   ref @0x" + Long.toHexString(from.getOffset()) +
                           (f == null ? " (no func)" : " in " + f.getName()));
                if (f != null) emit(f);
            }
        }

        // 2) scan all instructions for FourCC immediate operands
        long[] fcc = {0x54535250L, 0x706d534dL, 0x205a4156L, 0x2044494dL, 0x20525453L, 0x706d6153L};
        String[] fccN = {"PRST", "MSmp", "VAZ ", "MID ", "STR ", "Samp"};
        HashSet<Long> fccSet = new HashSet<>();
        for (long v : fcc) fccSet.add(v);
        pw.println("\n// ==== FourCC immediate operand matches ====");
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction ins = it.next();
            int no = ins.getNumOperands();
            for (int oi = 0; oi < no; oi++) {
                Scalar s = ins.getScalar(oi);
                if (s == null) continue;
                long v = s.getUnsignedValue() & 0xffffffffL;
                if (fccSet.contains(v)) {
                    Function f = getFunctionContaining(ins.getAddress());
                    String nm = "?";
                    for (int z = 0; z < fcc.length; z++) if (fcc[z] == v) nm = fccN[z];
                    pw.println("//   imm '" + nm + "' @0x" + Long.toHexString(ins.getAddress().getOffset()) +
                               (f == null ? " (none)" : " in " + f.getName()));
                    if (f != null) emit(f);
                }
            }
        }
        pw.close();
        println("FindLoader: wrote " + done.size() + " functions to " + out);
    }

    void emit(Function f) {
        long ea = f.getEntryPoint().getOffset();
        if (!done.add(ea)) return;
        DecompileResults res = di.decompileFunction(f, 60, monitor);
        pw.println("// ===== " + f.getName() + " @ 0x" + Long.toHexString(ea) + " =====");
        if (res != null && res.decompileCompleted()) pw.println(res.getDecompiledFunction().getC());
        else pw.println("// decompile failed");
        pw.println();
    }
}
