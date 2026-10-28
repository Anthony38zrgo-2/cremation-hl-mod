using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

internal static class CremationLauncher
{
    private const uint WmFontChange = 0x001D;
    private const uint SmtoAbortIfHung = 0x0002;
    private static readonly IntPtr HwndBroadcast = new IntPtr(0xffff);

    [DllImport("gdi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern int AddFontResourceW(string filename);

    [DllImport("gdi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool RemoveFontResourceW(string filename);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern IntPtr SendMessageTimeout(IntPtr hwnd, uint message, IntPtr wParam,
        IntPtr lParam, uint flags, uint timeout, out IntPtr result);

    private static void BroadcastFontChange()
    {
        IntPtr result;
        SendMessageTimeout(HwndBroadcast, WmFontChange, IntPtr.Zero, IntPtr.Zero,
            SmtoAbortIfHung, 1000, out result);
    }

    private static string SetSchemeFont(string source, string schemeName, string family)
    {
        string key = "\"" + schemeName + "\"";
        int blockStart = source.IndexOf(key, StringComparison.Ordinal);
        if (blockStart < 0 || source.IndexOf(key, blockStart + key.Length, StringComparison.Ordinal) >= 0)
            throw new InvalidDataException("No se encontro un bloque de fuente unico para " + schemeName);
        int blockOpen = source.IndexOf('{', blockStart + key.Length);
        if (blockOpen < 0) throw new InvalidDataException("Bloque de fuente invalido: " + schemeName);
        int depth = 0, blockEnd = -1;
        for (int i = blockOpen; i < source.Length; ++i)
        {
            if (source[i] == '{') ++depth;
            else if (source[i] == '}' && --depth == 0) { blockEnd = i + 1; break; }
        }
        if (blockEnd < 0) throw new InvalidDataException("Bloque de fuente incompleto: " + schemeName);
        int name = source.IndexOf("\"name\"", blockOpen, blockEnd - blockOpen, StringComparison.Ordinal);
        int valueStart = name < 0 ? -1 : source.IndexOf('"', name + 6, blockEnd - name - 6);
        int valueEnd = valueStart < 0 ? -1 : source.IndexOf('"', valueStart + 1, blockEnd - valueStart - 1);
        if (valueStart < 0 || valueEnd < 0)
            throw new InvalidDataException("No se encontro el nombre de fuente en " + schemeName);
        return source.Substring(0, valueStart + 1) + family + source.Substring(valueEnd);
    }

    [STAThread]
    private static int Main(string[] args)
    {
        string root = AppDomain.CurrentDomain.BaseDirectory;
        string engine = Path.Combine(root, "hl.exe");
        string mod = Path.Combine(root, "Cremation");
        bool valid = File.Exists(engine)
            && File.Exists(Path.Combine(mod, "liblist.gam"))
            && File.Exists(Path.Combine(mod, "maps", "cs_mansion.bsp"))
            && File.Exists(Path.Combine(mod, "dlls", "cremation.dll"))
            && File.Exists(Path.Combine(mod, "dlls", "hl_ravenholm.dll"))
            && File.Exists(Path.Combine(mod, "cl_dlls", "client.dll"))
            && File.Exists(Path.Combine(mod, "resource", "fonts", "Butcherman-Regular.ttf"))
            && File.Exists(Path.Combine(mod, "resource", "fonts", "Nosifer-Regular.ttf"))
            && File.Exists(Path.Combine(root, "valve", "resource", "TrackerScheme.res"));
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
        string[] fonts = {
            Path.Combine(mod, "resource", "fonts", "Butcherman-Regular.ttf"),
            Path.Combine(mod, "resource", "fonts", "Nosifer-Regular.ttf")
        };
        int loadedFonts = 0;
        string trackerScheme = Path.Combine(mod, "resource", "TrackerScheme.res");
        bool wroteTrackerScheme = false;
        bool hadTrackerScheme = File.Exists(trackerScheme);
        byte[] previousTrackerScheme = hadTrackerScheme ? File.ReadAllBytes(trackerScheme) : null;
        try
        {
            foreach (string font in fonts)
            {
                if (AddFontResourceW(font) == 0)
                    throw new InvalidOperationException("No se pudo cargar la fuente " + Path.GetFileName(font));
                loadedFonts++;
            }
            BroadcastFontChange();

            // Always derive from the complete stock scheme. A stale mod-local override may
            // contain only font entries, which makes VGUI build broken white panels.
            string baseSchemePath = Path.Combine(root, "valve", "resource", "TrackerScheme.res");
            string scheme = File.ReadAllText(baseSchemePath, Encoding.UTF8).TrimStart('\uFEFF');
            scheme = SetSchemeFont(scheme, "MenuLarge", "Butcherman");
            scheme = SetSchemeFont(scheme, "MenuBlurLarge", "Butcherman");
            wroteTrackerScheme = true;
            File.WriteAllText(trackerScheme, scheme, new UTF8Encoding(false));

            using (Process game = Process.Start(new ProcessStartInfo {
                FileName = engine,
                Arguments = "-game Cremation -num_edicts 2048 -console +exec cremation.cfg",
                WorkingDirectory = root,
                UseShellExecute = false
            }))
            {
                game.WaitForExit();
            }
            return 0;
        }
        catch (Exception ex)
        {
            MessageBox.Show("No se pudo iniciar Cremation:\n" + ex.Message, "Cremation", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 3;
        }
        finally
        {
            if (wroteTrackerScheme)
            {
                if (hadTrackerScheme) File.WriteAllBytes(trackerScheme, previousTrackerScheme);
                else if (File.Exists(trackerScheme)) File.Delete(trackerScheme);
            }
            for (int i = loadedFonts - 1; i >= 0; --i) RemoveFontResourceW(fonts[i]);
            if (loadedFonts > 0) BroadcastFontChange();
        }
    }
}
