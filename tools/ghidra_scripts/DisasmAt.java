// Dump raw disassembly (with constant refs) for the function containing each address.
// Args: outPath then hex addresses. Run against the kept project (-process -noanalysis).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.Reference;
import java.io.FileWriter;
import java.io.PrintWriter;

public class DisasmAt extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter pw = new PrintWriter(new FileWriter(a[0]));
        Listing lst = currentProgram.getListing();
        for (int k = 1; k < a.length; k++) {
            long va = Long.decode(a[k]);
            Function f = getFunctionContaining(toAddr(va));
            if (f == null) { try { disassemble(toAddr(va)); f = createFunction(toAddr(va), null); } catch (Exception e) {} }
            if (f == null) { pw.println("// no fn @0x" + Long.toHexString(va)); continue; }
            pw.println("// ==== " + f.getName() + " @0x" + Long.toHexString(f.getEntryPoint().getOffset()) + " ====");
            Address addr = f.getEntryPoint();
            Address end = f.getBody().getMaxAddress();
            Instruction insn = lst.getInstructionAt(addr);
            while (insn != null && insn.getAddress().compareTo(end) <= 0) {
                StringBuilder sb = new StringBuilder();
                sb.append("  ").append(insn.getAddress()).append("  ").append(insn.toString());
                for (Reference r : insn.getReferencesFrom())
                    if (r.getReferenceType().isData()) {
                        Address t = r.getToAddress();
                        try { sb.append("   ; [").append(t).append("]=f32:").append(getFloat(t)); } catch (Exception e) {}
                    }
                pw.println(sb.toString());
                insn = insn.getNext();
            }
            pw.println();
        }
        pw.close();
        println("DisasmAt done");
    }
}
