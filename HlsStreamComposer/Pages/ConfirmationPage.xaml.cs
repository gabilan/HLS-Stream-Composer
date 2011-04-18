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
using System.Threading;

namespace HlsStreamComposer
{
    /// <summary>
    /// Interaction logic for ConfirmationPage.xaml
    /// </summary>
    public partial class ConfirmationPage : Page
    {
        public ConfirmationPage()
        {
            InitializeComponent();
        }

        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            string errMsg = string.Empty;
            bool startTranscoding = true;

            if (!EncodingProcess.Current.ReadyForEncode(ref errMsg))
            {
                MessageBox.Show(errMsg, "Unable to begin encoding!", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                startTranscoding = false;
            }
            else if (MessageBox.Show("Are you sure that you'd like to begin encoding with these settings?", "Begin Encoding?", MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.No)
                startTranscoding = false;

            if (startTranscoding)
            {
                ObjectDataProvider objProv = this.Resources["objProv"] as ObjectDataProvider;
                if (objProv != null)
                    objProv.Refresh();

                EncodingProcess.Current.Start();
            }
            else
                NavigationService.GoBack();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (MessageBox.Show("Are you sure that you'd like to cancel the current encoding process?", "Cancel Encoding?", MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes)
            {
                EncodingProcess.Current.Stop();
                NavigationService.GoBack();
            }
        }
    }
}
