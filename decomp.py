from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor
dc = DecompInterface()
dc.openProgram(currentProgram)
mon = ConsoleTaskMonitor()
out = open("zaphkiel_decompiled.c", "w")
fm = currentProgram.getFunctionManager()
for f in fm.getFunctions(True):
    r = dc.decompileFunction(f, 30, mon)
    if r.decompileCompleted():
        out.write("\n// ===== %s =====\n" % f.getName())
        out.write(r.getDecompiledFunction().getC())
out.close()
