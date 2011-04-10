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
    /// Interaction logic for Window1.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        FileSelectionPage fsPage = new FileSelectionPage();
        EncodingOptionsPage eoPage = new EncodingOptionsPage();
        ConfirmationPage cPage = new ConfirmationPage();

        public MainWindow()
        {
            InitializeComponent();

            fsPage.Refresh();
            frameContent.Navigate(fsPage);
        }

        private void btnPrevious_Click(object sender, RoutedEventArgs e)
        {
            ISaveablePage saveable = frameContent.Content as ISaveablePage;
            if (saveable != null)
                saveable.Save();

            if (frameContent.Content == eoPage)
                frameContent.Navigate(fsPage);
            else if (frameContent.Content == cPage)
                frameContent.Navigate(eoPage);
        }

        private void btnNext_Click(object sender, RoutedEventArgs e)
        {
            ISaveablePage saveable = frameContent.Content as ISaveablePage;
            if (saveable != null)
                saveable.Save();

            if (frameContent.Content == fsPage)
                frameContent.Navigate(eoPage);
            else if (frameContent.Content == eoPage)
                frameContent.Navigate(cPage);
        }

        private void frameContent_Navigated(object sender, NavigationEventArgs e)
        {
            if (e.Content is FileSelectionPage)
                btnPrevious.IsEnabled = false;
            else
                btnPrevious.IsEnabled = true;
        }
    }
}
