using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace Wildline.Updates
{
    public sealed class Part { public string name; public long size; public string sha256; }
    public sealed class Manifest {
        public int schemaVersion; public string channel; public string version; public long sequence;
        public string tag; public long installerSize; public string installerSha256; public Part[] parts;
    }
    public sealed class Envelope { public string payload; public string signature; }
    public sealed class PayloadFile { public string path; public string sha256; }
    public sealed class BuildInfo { public string version; public long sequence; public PayloadFile[] files; }

    public static class Core
    {
        public const string Repository = "https://github.com/mviterna/wildline-downloads";
        public const string Feed = Repository + "/releases/download/beta/latest.json";
        public static readonly JavaScriptSerializer Json = new JavaScriptSerializer { MaxJsonLength = 8 * 1024 * 1024 };
        public static string PublicKey {
            get { using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("UpdatePublicKey.xml"))
                using (var r = new StreamReader(s)) return r.ReadToEnd(); }
        }
        public static bool SafeVersion(string value) { return value != null && Regex.IsMatch(value, @"^\d+\.\d+\.\d+-beta\.\d{8}\.\d{6}$"); }
        public static string Hash(string path) {
            using (var s = File.OpenRead(path)) using (var h = SHA256.Create())
                return BitConverter.ToString(h.ComputeHash(s)).Replace("-", "").ToLowerInvariant();
        }
        public static void CheckFile(string path, long size, string hash) {
            if (!File.Exists(path) || new FileInfo(path).Length != size || !String.Equals(Hash(path), hash, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Download verification failed: " + Path.GetFileName(path));
        }
        public static bool CachedPartValid(string path, Part part) {
            if (!File.Exists(path)) return false;
            try { CheckFile(path, part.size, part.sha256); return true; }
            catch (InvalidDataException) { return false; }
            catch (IOException) { return false; }
        }
        public static Manifest ReadManifest(string text, string publicKey) {
            var envelope = Json.Deserialize<Envelope>(text);
            byte[] data = Convert.FromBase64String(envelope.payload);
            using (var rsa = new RSACryptoServiceProvider()) {
                rsa.PersistKeyInCsp = false; rsa.FromXmlString(publicKey);
                if (!rsa.VerifyData(data, CryptoConfig.MapNameToOID("SHA256"), Convert.FromBase64String(envelope.signature)))
                    throw new InvalidDataException("The update signature is invalid.");
            }
            var m = Json.Deserialize<Manifest>(Encoding.UTF8.GetString(data));
            if (m.schemaVersion != 1 || m.channel != "beta" || !SafeVersion(m.version) || m.tag != "v" + m.version ||
                m.sequence <= 0 || m.installerSize <= 0 || m.parts == null || m.parts.Length == 0 || m.parts.Length > 100 ||
                !ValidHash(m.installerSha256)) throw new InvalidDataException("Unsupported update manifest.");
            long total = 0; var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var p in m.parts) {
                if (p.name == null || !Regex.IsMatch(p.name, @"^Wildline-[0-9A-Za-z.\-]+-Setup\.exe\.part\d{3}$") ||
                    !p.name.StartsWith("Wildline-" + m.version + "-Setup.exe.part", StringComparison.Ordinal) ||
                    p.size <= 0 || p.size >= 2L * 1024 * 1024 * 1024 || !ValidHash(p.sha256) || !names.Add(p.name))
                    throw new InvalidDataException("Invalid update part.");
                total = checked(total + p.size);
            }
            if (total != m.installerSize) throw new InvalidDataException("Incorrect update size.");
            return m;
        }
        static bool ValidHash(string hash) { return hash != null && Regex.IsMatch(hash, "^[a-fA-F0-9]{64}$"); }
        public static string VersionRoot(string root, string version) {
            if (!SafeVersion(version)) throw new InvalidDataException("Invalid installed version.");
            return Path.Combine(root, "versions", version);
        }
        public static string ActiveVersion(string root) {
            string path = Path.Combine(root, "current.version");
            return File.Exists(path) ? File.ReadAllText(path).Trim() : null;
        }
        public static string GameRoot(string root) {
            string version = ActiveVersion(root);
            if (version != null && version != "legacy") return VersionRoot(root, version);
            return root; // Migrate installations made before the launcher existed.
        }
        public static BuildInfo Installed(string root) {
            var path = Path.Combine(GameRoot(root), "BetaBuild.json");
            return File.Exists(path) ? Json.Deserialize<BuildInfo>(File.ReadAllText(path)) : null;
        }
        public static void AtomicWrite(string path, string contents, string backup) {
            string temp = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try {
                File.WriteAllText(temp, contents, new UTF8Encoding(false));
                if (File.Exists(path)) File.Replace(temp, path, backup); else File.Move(temp, path);
            } finally { if (File.Exists(temp)) File.Delete(temp); }
        }
        public static void VerifyPayload(string folder, string version) {
            var info = Json.Deserialize<BuildInfo>(File.ReadAllText(Path.Combine(folder, "BetaBuild.json")));
            if (info.version != version || info.files == null || info.files.Length == 0) throw new InvalidDataException("Incomplete game payload.");
            string prefix = Path.GetFullPath(folder).TrimEnd('\\') + "\\";
            foreach (var f in info.files) {
                string file = Path.GetFullPath(Path.Combine(folder, f.path));
                if (!file.StartsWith(prefix, StringComparison.OrdinalIgnoreCase) || !ValidHash(f.sha256) ||
                    !File.Exists(file) || !String.Equals(Hash(file), f.sha256, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("Game verification failed: " + f.path);
            }
            foreach (string name in new[] { "Wildline.exe", @"Wildline\Binaries\Win64\Wildline.exe", @"Engine\Build\InstalledProjectBuild.txt" })
                if (!File.Exists(Path.Combine(folder, name))) throw new InvalidDataException("Game file missing: " + name);
        }
        public static void Activate(string root, string version) {
            VerifyPayload(VersionRoot(root, version), version);
            if (ActiveVersion(root) == null && File.Exists(Path.Combine(root, "Wildline.exe")) && File.Exists(Path.Combine(root, "BetaBuild.json")))
                AtomicWrite(Path.Combine(root, "previous.version"), "legacy", null);
            if (ActiveVersion(root) != version) AtomicWrite(Path.Combine(root, "current.version"), version, Path.Combine(root, "previous.version"));
        }
        public static void Rollback(string root) {
            string version = File.ReadAllText(Path.Combine(root, "previous.version")).Trim();
            if (version == "legacy") {
                var info = Json.Deserialize<BuildInfo>(File.ReadAllText(Path.Combine(root,"BetaBuild.json")));
                VerifyPayload(root,info.version);
                AtomicWrite(Path.Combine(root,"current.version"),"legacy",Path.Combine(root,"previous.version")); return;
            }
            Activate(root, version);
        }
        public static bool GameRunning(string root) {
            string prefix = Path.GetFullPath(root).TrimEnd('\\') + "\\";
            foreach (var p in Process.GetProcessesByName("Wildline")) {
                try { if (p.MainModule.FileName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) return true; }
                catch (System.ComponentModel.Win32Exception) { return true; }
                catch (InvalidOperationException) { }
                finally { p.Dispose(); }
            }
            return false;
        }
        public static void Launch(string root) {
            string folder = GameRoot(root), game = Path.Combine(folder, "Wildline.exe");
            if (!File.Exists(game)) throw new FileNotFoundException("No installed game is available. Connect to the internet to install WILDLINE.");
            Process.Start(new ProcessStartInfo(game, "-preferNvidia") { WorkingDirectory = folder, UseShellExecute = true });
        }
        public static string Cache(Manifest m) {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Wildline", "Updates", m.version);
        }
        public static void Assemble(Manifest m, string cache, Action<string, int> progress) {
            string setup = Path.Combine(cache, "Setup.exe"), temp = setup + ".tmp";
            using (var output = new FileStream(temp, FileMode.Create, FileAccess.Write, FileShare.None)) {
                for (int i = 0; i < m.parts.Length; i++) {
                    var part = m.parts[i]; string path = Path.Combine(cache, part.name);
                    CheckFile(path, part.size, part.sha256);
                    progress("Preparing installation...", (i + 1) * 100 / m.parts.Length);
                    using (var input = File.OpenRead(path)) input.CopyTo(output);
                }
            }
            CheckFile(temp, m.installerSize, m.installerSha256);
            if (File.Exists(setup)) File.Delete(setup);
            File.Move(temp, setup);
        }
        public static string Quote(string value) {
            if (value.Contains("\"") || value.Contains("\r") || value.Contains("\n")) throw new ArgumentException("Invalid path argument.");
            return "\"" + value.TrimEnd('\\') + "\"";
        }
    }

    sealed class LauncherWindow : Form
    {
        readonly Label status = new Label(); readonly ProgressBar bar = new ProgressBar();
        readonly Button play = new Button(); readonly Button retry = new Button(); readonly Button rollback = new Button();
        readonly string root; bool working; bool exitForHelper;
        public LauncherWindow(string installRoot) {
            root = installRoot; Text = "WILDLINE"; ClientSize = new Size(540, 230); FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false; StartPosition = FormStartPosition.CenterScreen; BackColor = Color.FromArgb(24, 34, 29); ForeColor = Color.White;
            var title = new Label { Text = "WILDLINE", Font = new Font("Segoe UI", 26, FontStyle.Bold), Location = new Point(24, 18), AutoSize = true };
            status.SetBounds(28, 82, 480, 42); status.Text = "Checking for updates...";
            bar.SetBounds(28, 132, 480, 12);
            play.Text = "Play installed"; play.SetBounds(28, 171, 145, 32); play.Enabled = false;
            retry.Text = "Retry update"; retry.SetBounds(185, 171, 145, 32); retry.Visible = false;
            rollback.Text = "Previous version"; rollback.SetBounds(342, 171, 166, 32); rollback.Visible = false;
            foreach (var button in new[] { play, retry, rollback }) { button.ForeColor = Color.Black; button.BackColor = Color.White; }
            Controls.AddRange(new Control[] { title, status, bar, play, retry, rollback });
            play.Click += delegate { try { Core.Launch(root); Close(); } catch (Exception e) { status.Text = e.Message; } };
            retry.Click += async delegate { await RunUpdate(); };
            rollback.Click += delegate { try { Core.Rollback(root); Core.Launch(root); Close(); } catch (Exception e) { status.Text = e.Message; } };
            Shown += async delegate { await RunUpdate(); };
            FormClosing += delegate(object sender, FormClosingEventArgs e) { if (working && !exitForHelper) e.Cancel = true; };
        }
        void Report(string message, int percent) { Invoke((Action)delegate { status.Text = message; bar.Value = Math.Max(0, Math.Min(100, percent)); }); }
        async Task RunUpdate() {
            working = true; retry.Visible = false; play.Enabled = false; rollback.Visible = false;
            try {
                bool helper = await Task.Run(() => FetchUpdate());
                working = false;
                if (!helper) Core.Launch(root);
                Close();
            } catch (Exception e) {
                working = false; status.Text = "Update unavailable: " + e.Message;
                if (e is WebException && File.Exists(Path.Combine(Core.GameRoot(root), "Wildline.exe"))) {
                    Core.Launch(root); Close(); return;
                }
                Directory.CreateDirectory(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Wildline", "LauncherLogs"));
                File.AppendAllText(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Wildline", "LauncherLogs", "launcher.log"), DateTime.UtcNow.ToString("o") + " " + e + Environment.NewLine);
                play.Enabled = File.Exists(Path.Combine(Core.GameRoot(root), "Wildline.exe")); retry.Visible = true;
                rollback.Visible = File.Exists(Path.Combine(root, "previous.version"));
            }
        }
        bool FetchUpdate() {
            var installed = Core.Installed(root);
            string envelope;
            using (var client = new TimedClient()) envelope = client.DownloadString(Core.Feed);
            var m = Core.ReadManifest(envelope, Core.PublicKey);
            if (installed != null && installed.sequence >= m.sequence && File.Exists(Path.Combine(Core.GameRoot(root), "Wildline.exe"))) return false;
            if (Core.GameRunning(root)) throw new InvalidOperationException("Close the running game, then retry.");
            string cache = Core.Cache(m); Directory.CreateDirectory(cache);
            // Cache and installer coexist while setup writes a new version. Check both volumes.
            long downloadSpace = checked(2 * m.installerSize + 512L * 1024 * 1024);
            if (new DriveInfo(Path.GetPathRoot(cache)).AvailableFreeSpace < downloadSpace)
                throw new IOException("Free at least " + (downloadSpace / (1024 * 1024 * 1024) + 1) + " GB for the download.");
            for (int i = 0; i < m.parts.Length; i++) {
                var part = m.parts[i]; string path = Path.Combine(cache, part.name);
                bool valid = Core.CachedPartValid(path, part);
                if (!valid) {
                    string temp = path + ".download";
                    using (var client = new TimedClient()) {
                        client.DownloadProgressChanged += delegate(object sender, DownloadProgressChangedEventArgs e) {
                            Report("Downloading update " + (i + 1) + " of " + m.parts.Length + " (" + e.ProgressPercentage + "%)", (i * 100 + e.ProgressPercentage) / m.parts.Length);
                        };
                        var download = client.DownloadFileTaskAsync(new Uri(Core.Repository + "/releases/download/" + m.tag + "/" + part.name), temp);
                        long lastBytes = 0; DateTime lastProgress = DateTime.UtcNow;
                        client.DownloadProgressChanged += delegate(object sender, DownloadProgressChangedEventArgs e) {
                            if (e.BytesReceived != Interlocked.Read(ref lastBytes)) { Interlocked.Exchange(ref lastBytes, e.BytesReceived); lastProgress = DateTime.UtcNow; }
                        };
                        while (!download.Wait(1000)) {
                            if (DateTime.UtcNow - lastProgress > TimeSpan.FromSeconds(45)) { client.CancelAsync(); throw new WebException("Download stalled. Retry to reuse completed parts."); }
                        }
                        download.GetAwaiter().GetResult();
                    }
                    Core.CheckFile(temp, part.size, part.sha256);
                    if (File.Exists(path)) File.Delete(path); File.Move(temp, path);
                }
                Report("Verified download " + (i + 1) + " of " + m.parts.Length, (i + 1) * 100 / m.parts.Length);
            }
            Core.Assemble(m, cache, Report);
            File.WriteAllText(Path.Combine(cache, "latest.json"), envelope);
            string helper = Path.Combine(cache, "UpdateHelper.exe");
            File.Copy(Assembly.GetExecutingAssembly().Location, helper, true);
            Process.Start(new ProcessStartInfo(helper, "--apply " + Core.Quote(root) + " " + Core.Quote(Path.Combine(cache, "latest.json")) + " " + Process.GetCurrentProcess().Id) { UseShellExecute = false });
            exitForHelper = true; return true;
        }
    }
    sealed class TimedClient : WebClient {
        public TimedClient() { Headers[HttpRequestHeader.UserAgent] = "WildlineLauncher/1.0"; Encoding = Encoding.UTF8; }
        protected override WebRequest GetWebRequest(Uri address) {
            var request = base.GetWebRequest(address); request.Timeout = 20000;
            var http = request as HttpWebRequest; if (http != null) http.ReadWriteTimeout = 30000;
            return request;
        }
    }
    static class Program
    {
        [STAThread] static int Main(string[] args) {
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
            string root = AppDomain.CurrentDomain.BaseDirectory.TrimEnd('\\');
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            try {
                if (args.Length == 2 && args[0] == "--activate") { Core.Activate(root, args[1]); return 0; }
                if (args.Length == 2 && args[0] == "--verify-manifest") { Core.ReadManifest(File.ReadAllText(args[1]), Core.PublicKey); return 0; }
                if (args.Length == 1 && args[0] == "--verify-only") { Core.VerifyPayload(Core.GameRoot(root), Core.ActiveVersion(root)); return 0; }
                if (args.Length == 4 && args[0] == "--apply") return Apply(args);
                if (args.Length == 1 && args[0] == "--rollback") { Core.Rollback(root); Core.Launch(root); return 0; }
                if (args.Length == 1 && args[0] == "--offline") { Core.Launch(root); return 0; }
                using (var mutex = new Mutex(false, "Local\\WildlineLauncher")) {
                    if (!mutex.WaitOne(0)) return 0;
                    try { Application.Run(new LauncherWindow(root)); } finally { mutex.ReleaseMutex(); }
                }
                return 0;
            } catch (Exception e) {
                if (args.Length > 0 && (args[0] == "--activate" || args[0].StartsWith("--verify"))) return 1;
                MessageBox.Show(e.Message, "WILDLINE", MessageBoxButtons.OK, MessageBoxIcon.Error); return 1;
            }
        }
        static int Apply(string[] args) {
            string root = Path.GetFullPath(args[1]);
            var m = Core.ReadManifest(File.ReadAllText(args[2]), Core.PublicKey);
            string cache = Core.Cache(m), setup = Path.Combine(cache, "Setup.exe");
            if (!String.Equals(Path.GetFullPath(args[2]), Path.Combine(cache, "latest.json"), StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Invalid update cache.");
            int parentId = Int32.Parse(args[3]);
            try { using (var parent = Process.GetProcessById(parentId)) { if (!parent.WaitForExit(30000)) throw new IOException("Launcher did not close."); } } catch (ArgumentException) { }
            using (var mutex = new Mutex(false, "Local\\WildlineLauncher")) {
                if (!mutex.WaitOne(0)) throw new IOException("Another WILDLINE launcher is open.");
                try {
                    if (Core.GameRunning(root)) throw new IOException("Close WILDLINE before installing the update.");
                    var installed = Core.Installed(root);
                    if (installed != null && installed.sequence >= m.sequence) { Core.Launch(root); return 0; }
                    Core.CheckFile(setup, m.installerSize, m.installerSha256);
                    string log = Path.Combine(cache, "install.log");
                    using (var process = Process.Start(new ProcessStartInfo(setup, "/SILENT /SUPPRESSMSGBOXES /NORESTART /RESTARTEXITCODE=3010 /SP- /DIR=" + Core.Quote(root) + " /LOG=" + Core.Quote(log)) { UseShellExecute = true })) {
                        process.WaitForExit();
                        if (process.ExitCode == 3010) { MessageBox.Show("Restart Windows, then open WILDLINE to play.", "WILDLINE"); return 0; }
                        if (process.ExitCode != 0) throw new IOException("Installation failed (" + process.ExitCode + "). Your previous version is retained. Log: " + log);
                    }
                    if (Core.ActiveVersion(root) != m.version) throw new IOException("The update did not activate. Your previous version is retained.");
                    // Keep the receipt and log, remove only this update's verified download files.
                    foreach (var part in m.parts) File.Delete(Path.Combine(cache, part.name));
                    File.Delete(setup);
                    Core.Launch(root); return 0;
                } finally { mutex.ReleaseMutex(); }
            }
        }
    }
}
