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

namespace HlsStreamComposer
{
    /// <summary>
    /// Interaction logic for PlaylistOptionsPage.xaml
    /// </summary>
    public partial class PlaylistOptionsPage : Page, ISaveablePage
    {
        public PlaylistOptionsPage()
        {
            InitializeComponent();
        }

        public void Save()
        {
            var options = EncodingProcess.Current.SegmentOptions;
            int segmentDuration;

            if (int.TryParse(tbSegmentDuration.Text, out segmentDuration))
                options.SegmentLength = segmentDuration;
            else
            {
                options.SegmentLength = 10;
                tbSegmentDuration.Text = "10";
            }

            options.FilenamePrefix = tbPlaylistUrl.Text;
        }
    }
}
