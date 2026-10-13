# DecompileAll.py - UTF-8 fixed
# -*- coding: utf-8 -*-
import io
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

decomp = DecompInterface()
decomp.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

f = io.open("/tmp/decompiled_output.c", "w", encoding="utf-8")
count = 0
for func in currentProgram.getFunctionManager().getFunctions(True):
    try:
        res = decomp.decompileFunction(func, 60, monitor)
        if res.decompileCompleted():
            code = res.getDecompiledFunction().getC()
            f.write(u"// Function: " + func.getName() + u" @ " + unicode(func.getEntryPoint()) + u"\n")
            f.write(code)
            f.write(u"\n// ----------------------------------------\n\n")
            count += 1
    except Exception as e:
        pass
f.close()
print("Decompiled %d functions. Output: /tmp/decompiled_output.c" % count)
