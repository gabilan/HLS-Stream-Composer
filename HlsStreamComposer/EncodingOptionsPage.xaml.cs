using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;

namespace HlsStreamComposer
{
    /// <summary>
    /// Interaction logic for EncodingOptionsPage.xaml
    /// </summary>
    public partial class EncodingOptionsPage : Page
    {
        public EncodingOptionsPage()
        {
            InitializeComponent();

            foreach (var file in Directory.GetFiles(@"Lib\presets", "*.ffpreset"))
            {
                if (!file.Contains("libx264-"))
                    continue;

                string presetName = Cleanse(file);
                string presetDisplayText = NormalizeText(presetName);

                ComboBoxItem cbItem = new ComboBoxItem() { Content = presetDisplayText, Tag = presetName };
                cbVideoPreset.Items.Add(cbItem);
            }

            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "192k" });
            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "128k" });
            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "96k" });
            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "64k" });

            cbAudioSampleRate.Items.Add(new ComboBoxItem() { Content = "48000" });
            cbAudioSampleRate.Items.Add(new ComboBoxItem() { Content = "44100" });

            cbAudioChannels.Items.Add(new ComboBoxItem() { Content = "2" });
            cbAudioChannels.Items.Add(new ComboBoxItem() { Content = "1" });
        }

        private string Cleanse(string file)
        {
            file = System.IO.Path.GetFileNameWithoutExtension(file);

            int index = file.IndexOf("libx264-");
            if (index != -1)
                file = file.Remove(0, index + "libx264-".Length);

            return file;
        }

        string NormalizeText(string s)
        {
            List<char> chars = new List<char>();
            bool closeParen = false;
            bool lastCharNum = false;

            s = s.Replace("firstpass", "First pass");
            s = s.Replace("ultrafast", "ultra fast");
            s = s.Replace("superfast", "super fast");
            s = s.Replace("veryfast", "very fast");
            s = s.Replace("veryslow", "very slow");

            for (int i = 0; i < s.Length; i++)
            {
                if (i == 0)
                    chars.Add(char.ToUpper(s[i]));
                else if (char.IsDigit(s[i]))
                {
                    if (!lastCharNum)
                        chars.Add(' ');

                    lastCharNum = true;
                    chars.Add(s[i]);
                }
                else if (s[i] == '_')
                {
                    closeParen = true;
                    chars.Add(' ');
                    chars.Add('(');
                }
                else
                    chars.Add(s[i]);
            }

            if (closeParen)
                chars.Add(')');

            return new string(chars.ToArray());
        }
    }
}
