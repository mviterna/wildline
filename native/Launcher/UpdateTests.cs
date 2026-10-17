using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using Wildline.Updates;

static class UpdateTests
{
    static int passed;
    static void Check(bool ok, string name) { if (!ok) throw new Exception(name); passed++; Console.WriteLine("PASS " + name); }
    static void Reject(Action action, string name) {
        bool rejected = false; try { action(); } catch { rejected = true; } Check(rejected, name);
    }
    static string Signed(Manifest m, RSACryptoServiceProvider rsa) {
        byte[] bytes = Encoding.UTF8.GetBytes(Core.Json.Serialize(m));
        return Core.Json.Serialize(new Envelope { payload = Convert.ToBase64String(bytes), signature = Convert.ToBase64String(rsa.SignData(bytes, CryptoConfig.MapNameToOID("SHA256"))) });
    }
    static void Fixture(string root, string version, long sequence) {
        string dir = Core.VersionRoot(root, version); Directory.CreateDirectory(dir);
        string[] names = { "Wildline.exe", @"Wildline\Binaries\Win64\Wildline.exe", @"Engine\Build\InstalledProjectBuild.txt", "BetaReadme.txt" };
        var files = new PayloadFile[names.Length];
        for (int i = 0; i < names.Length; i++) {
            string path = Path.Combine(dir, names[i]); Directory.CreateDirectory(Path.GetDirectoryName(path)); File.WriteAllText(path, version);
            files[i] = new PayloadFile { path = names[i], sha256 = Core.Hash(path) };
        }
        File.WriteAllText(Path.Combine(dir, "BetaBuild.json"), Core.Json.Serialize(new BuildInfo { version = version, sequence = sequence, files = files }));
    }
    static int Main(string[] args) {
        string root = Path.Combine(Path.GetFullPath(args[0]), Guid.NewGuid().ToString("N")); Directory.CreateDirectory(root);
        string a = "0.2.0-beta.20261015.120000", b = "0.2.0-beta.20261016.120000";
        try {
            using (var rsa = new RSACryptoServiceProvider(2048)) {
                rsa.PersistKeyInCsp = false;
                string p = "Wildline-" + b + "-Setup.exe.part000";
                File.WriteAllText(Path.Combine(root,p), "verified installer bytes");
                var m = new Manifest { schemaVersion = 1, channel = "beta", version = b, sequence = 10, tag = "v" + b,
                    installerSize = new FileInfo(Path.Combine(root,p)).Length, installerSha256 = Core.Hash(Path.Combine(root,p)),
                    parts = new[] { new Part { name = p, size = new FileInfo(Path.Combine(root,p)).Length, sha256 = Core.Hash(Path.Combine(root,p)) } } };
                string envelope = Signed(m,rsa), key = rsa.ToXmlString(false);
                Check(!Core.CachedPartValid(Path.Combine(root,"missing.part"),m.parts[0]), "empty first-install cache requests download");
                Check(Core.CachedPartValid(Path.Combine(root,p),m.parts[0]), "verified cached part can be reused");
                Check(Core.ReadManifest(envelope,key).version == b, "signed beta accepted");
                using (var wrong = new RSACryptoServiceProvider(2048)) { wrong.PersistKeyInCsp = false; Reject(() => Core.ReadManifest(envelope,wrong.ToXmlString(false)), "wrong key rejected"); }
                var changed = Core.Json.Deserialize<Envelope>(envelope); changed.payload = Convert.ToBase64String(Encoding.UTF8.GetBytes("{}"));
                Reject(() => Core.ReadManifest(Core.Json.Serialize(changed),key), "manifest tampering rejected");
                m.channel = "stable"; Reject(() => Core.ReadManifest(Signed(m,rsa),key), "wrong channel rejected"); m.channel = "beta";
                m.parts[0].name = "../setup.exe"; Reject(() => Core.ReadManifest(Signed(m,rsa),key), "part path traversal rejected"); m.parts[0].name = p;
                m.parts[0].size = 2L * 1024 * 1024 * 1024; Reject(() => Core.ReadManifest(Signed(m,rsa),key), "GitHub oversized part rejected"); m.parts[0].size = m.installerSize;
                m.tag = "beta"; Reject(() => Core.ReadManifest(Signed(m,rsa),key), "mutable asset tag rejected"); m.tag = "v" + b;
                m.installerSize++; Reject(() => Core.ReadManifest(Signed(m,rsa),key), "incorrect total size rejected"); m.installerSize--;
                Core.Assemble(m,root,(s,n)=>{}); Check(Core.Hash(Path.Combine(root,"Setup.exe")) == m.installerSha256, "parts reconstructed and full installer verified");
                File.WriteAllText(Path.Combine(root,p),new string('x',(int)m.installerSize));
                Check(!Core.CachedPartValid(Path.Combine(root,p),m.parts[0]), "corrupt cached part requests fresh download");
                Reject(() => Core.Assemble(m,root,(s,n)=>{}), "corrupt cached part rejected");
            }
            Check(Core.GameRoot(root) == root, "legacy install recognized");
            Reject(() => Core.VersionRoot(root,"../../outside"), "installed version traversal rejected");
            Fixture(root,a,1); Core.Activate(root,a); Check(Core.ActiveVersion(root) == a, "verified version activated");
            Fixture(root,b,2); File.WriteAllText(Path.Combine(Core.VersionRoot(root,b),"Wildline.exe"),"corrupt");
            Reject(() => Core.Activate(root,b), "incomplete or corrupt installation rejected");
            Check(Core.ActiveVersion(root) == a, "failed update retains previous selection");
            Fixture(root,b,2); Core.Activate(root,b);
            Check(Core.ActiveVersion(root) == b && File.ReadAllText(Path.Combine(root,"previous.version")) == a, "successful update keeps previous version");
            Core.Rollback(root); Check(Core.ActiveVersion(root) == a && Core.Installed(root).sequence == 1, "rollback restores verified previous version");
            string save = Path.Combine(root,"Wildline","Saved","profile.txt"); Directory.CreateDirectory(Path.GetDirectoryName(save)); File.WriteAllText(save,"keep");
            Core.Activate(root,b); Check(File.ReadAllText(save) == "keep", "activation preserves saves");
            string legacy = Path.Combine(root,"legacy install"); Fixture(legacy,a,1);
            string source = Core.VersionRoot(legacy,a);
            foreach (string file in Directory.GetFiles(source,"*",SearchOption.AllDirectories)) {
                string target = Path.Combine(legacy,file.Substring(source.Length+1)); Directory.CreateDirectory(Path.GetDirectoryName(target)); File.Copy(file,target);
            }
            Fixture(legacy,b,2); Core.Activate(legacy,b);
            Check(File.ReadAllText(Path.Combine(legacy,"previous.version")) == "legacy", "migration retains legacy rollback selection");
            Core.Rollback(legacy); Check(Core.GameRoot(legacy) == legacy && Core.Installed(legacy).version == a, "legacy rollback verifies and restores old game");
            Console.WriteLine("PASS: " + passed + " updater checks. Fixtures: " + root); return 0;
        } catch (Exception e) { Console.Error.WriteLine(e); return 1; }
    }
}
