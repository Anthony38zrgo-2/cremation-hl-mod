using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

internal static class CremationLauncher
{
    [STAThread]
    private static int Main(string[] args)
    {
        string root = AppDomain.CurrentDomain.BaseDirectory;
        string engine = Path.Combine(root, "hl.exe");
        string mod = Path.Combine(root, "Cremation");
        bool valid = File.Exists(engine)
            && File.Exists(Path.Combine(mod, "liblist.gam"))
            && File.Exists(Path.Combine(mod, "maps", "rapidcore.bsp"))
            && File.Exists(Path.Combine(mod, "dlls", "cremation.dll"))
            && File.Exists(Path.Combine(mod, "dlls", "hl_ravenholm.dll"))
            && File.Exists(Path.Combine(mod, "cl_dlls", "client.dll"));
        if (args.Length == 1 && args[0] == "--verify") return valid ? 0 : 1;
        if (!valid)
        {
            MessageBox.Show("Coloca Cremation.exe junto a hl.exe y a la carpeta Cremation.", "Cremation", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
        if (Process.GetProcessesByName("hl").Length != 0)
        {
            MessageBox.Show("Cierra Half-Life antes de iniciar Cremation.", "Cremation", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return 2;
        }
        try
        {
            Process.Start(new ProcessStartInfo {
                FileName = engine,
                Arguments = "-game Cremation -console +exec cremation.cfg",
                WorkingDirectory = root,
                UseShellExecute = false
            });
            return 0;
        }
        catch (Exception ex)
        {
            MessageBox.Show("No se pudo iniciar Cremation:\n" + ex.Message, "Cremation", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 3;
        }
    }
}
