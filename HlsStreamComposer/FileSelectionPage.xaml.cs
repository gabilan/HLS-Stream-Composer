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
//using Microsoft.Win32;
using System.Windows.Forms;

namespace HlsStreamComposer
{
    /// <summary>
    /// Interaction logic for FileSelectionPage.xaml
    /// </summary>
    public partial class FileSelectionPage : Page
    {
        public string InputPath { get { return tbInputFile.Text; } }
        public string OutputPath { get { return tbOutputDirectory.Text; } }

        public FileSelectionPage()
        {
            InitializeComponent();
        }

        private void btnInputBrowse_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.CheckFileExists = true;
            ofd.CheckPathExists = true;

            if (ofd.ShowDialog() == DialogResult.OK)
                tbInputFile.Text = ofd.FileName;
        }

        private void btnOutputBrowse_Click(object sender, RoutedEventArgs e)
        {
            FolderBrowserDialog dlg = new FolderBrowserDialog();
            System.Windows.Forms.DialogResult result = dlg.ShowDialog(this.GetIWin32Window());
            if (result == System.Windows.Forms.DialogResult.OK)
                tbOutputDirectory.Text = dlg.SelectedPath;
        }
    }
}
