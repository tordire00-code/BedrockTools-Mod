// Ghidra script to decompile all functions
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.FileWriter;
import java.io.PrintWriter;

public class DecompileAll extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        PrintWriter writer = new PrintWriter(new FileWriter("/tmp/decompiled_output.c"));

        FunctionIterator funcs = currentProgram.getListing().getFunctions(true);
        while (funcs.hasNext()) {
            Function func = funcs.next();
            DecompileResults res = decomp.decompileFunction(func, 60, monitor);
            if (res.decompileCompleted()) {
                writer.println("// Function: " + func.getName() + " @ " + func.getEntryPoint());
                writer.println(res.getDecompiledFunction().getC());
                writer.println("\n// ----------------------------------------\n");
            }
        }
        writer.close();
        decomp.dispose();
        println("Decompilation finished. Output in /tmp/decompiled_output.c");
    }
}
