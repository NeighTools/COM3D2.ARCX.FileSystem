using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using Mono.Cecil;
using Mono.Cecil.Cil;

namespace COM3D2.ARCXLoader.Patcher
{
    public class ARCXLoaderPatcher
    {
        public static readonly string[] TargetAssemblyNames = {"Assembly-CSharp.dll"};

        public static void Patch(AssemblyDefinition ass)
        {
            var sceneStart = ass.MainModule.GetType("GameUty");
            var start = sceneStart.Methods.First(m => m.Name == ".cctor");

            var il = start.Body.GetILProcessor();
            var ins = start.Body.Instructions.First();

            il.InsertBefore(ins,
                            il.Create(OpCodes.Call,
                                      ass.MainModule.Import(typeof(ARCXLoader).GetMethod("Run",
                                                                                         BindingFlags.Public | BindingFlags.Static))));
        }
    }

    public class ARCXLoader
    {
        private static Action installArcX;

        public static void Run()
        {
            Console.WriteLine("[ARCX] Loader started");

            string nativeDll = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(typeof(ARCXLoader).Assembly.Location), "ARCXFS.dll"));

            var dll = LoadLibrary(nativeDll);

            if (dll == IntPtr.Zero)
            {
                Console.WriteLine($"[ARCX] ERROR: Failed to find {nativeDll}");
                return;
            }

            var installProc = GetProcAddress(dll, "InstallArcX");

            if (installProc == IntPtr.Zero)
            {
                Console.WriteLine($"[ARCX] ERROR: Invalid ARCXFS.dll");
                return;
            }

            installArcX = (Action) Marshal.GetDelegateForFunctionPointer(installProc, typeof(Action));

            Console.WriteLine("[ARCX] Installing filesystem");
            installArcX();
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr LoadLibrary(string path);

        [DllImport("kernel32.dll")]
        private static extern IntPtr GetProcAddress(IntPtr module, [MarshalAs(UnmanagedType.LPStr)] string proc);
    }
}