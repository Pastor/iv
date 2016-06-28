using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using SupController.Model;
using System;
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using WebSocketSharp;

namespace SupController
{
    public partial class SupMainForm : Form
    {
        private readonly JsonSerializerSettings settings = new JsonSerializerSettings();
        private WebSocket websocket;
        private NotifyIcon notifyIcon = new NotifyIcon();


        public SupMainForm()
        {
            InitializeComponent();

            var menu = new ContextMenu();
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("&Exit", (o, e) => {
                Close();
                Dispose();
                websocket.Close();
            });

            notifyIcon.Click += NotifyIcon_Click;
            notifyIcon.DoubleClick += NotifyIcon_DoubleClick;
            var content = Properties.SupMainResource.server_lightning;
            notifyIcon.Icon = Icon.FromHandle(content.GetHicon());
            notifyIcon.ContextMenu = menu;
            notifyIcon.Visible = true;
            //WindowState = FormWindowState.Minimized;

            settings.ContractResolver = new CamelCasePropertyNamesContractResolver();
                       
        }

        private void NotifyIcon_DoubleClick(object sender, EventArgs e)
        {
            Show();
            WindowState = FormWindowState.Normal;
        }

        private void NotifyIcon_Click(object sender, EventArgs e)
        {
            Show();
            WindowState = FormWindowState.Normal;
        }

        private void SupMainForm_Closing(object sender, FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing) {
                e.Cancel = true;
                Hide();
            }
        }

        private void SupMainForm_Load(object sender, EventArgs ev)
        {

            var offset = 0;
            var btnRewind = new ControlButton(Properties.SupMainResource.control_start, Properties.SupMainResource.control_start_blue);
            btnRewind.Location = new Point(offset + 10, 45);
            Controls.Add(btnRewind);

            var btnPlay = new ControlButton(Properties.SupMainResource.control_play, Properties.SupMainResource.control_play_blue);
            btnPlay.Location = new Point(offset + 50, 45);
            Controls.Add(btnPlay);

            var btnForward = new ControlButton(Properties.SupMainResource.control_end, Properties.SupMainResource.control_end_blue);
            btnForward.Location = new Point(offset + 90, 45);
            Controls.Add(btnForward);

            btnTransmit.Image = Properties.SupMainResource.transmit;

            var timeVisual = new TimerVisual();
            timeVisual.Location = new Point(8, 10);
            Controls.Add(timeVisual);

            var tooltip = new ToolTip();
            websocket = new WebSocket("ws://localhost:8080/notification");
            websocket.OnOpen += (ss, ee) => {
                btnTransmit.Image = Properties.SupMainResource.transmit_blue;
                notifyIcon.BalloonTipText = "Подключен";
                tooltip.SetToolTip(btnTransmit, "Подключен");
                btnForward.Enabled = true;
                btnRewind.Enabled = true;
                btnPlay.Enabled = true;
            };
            websocket.OnClose += (ss, ee) => {
                btnTransmit.Image = Properties.SupMainResource.transmit;
                notifyIcon.BalloonTipText = "Отключен";
                tooltip.SetToolTip(btnTransmit, "Отключен");
                btnForward.Enabled = false;
                btnRewind.Enabled = false;
                btnPlay.Enabled = false;
            };
            websocket.OnError += (ss, ee) => {
                btnTransmit.Image = Properties.SupMainResource.transmit_error;
                notifyIcon.BalloonTipText = "Ошибка";
                tooltip.SetToolTip(btnTransmit, "Ошибка");
                btnForward.Enabled = false;
                btnRewind.Enabled = false;
                btnPlay.Enabled = false;
            };
            websocket.OnMessage += (ss, ee) => {
                Debug.WriteLine("Message {0}", ee.Data);
                var e = JsonConvert.DeserializeObject<Event>(ee.Data, settings);
                if (e.Type == EventType.QUESTION_PROCESS) {
                    var pe = JsonConvert.DeserializeObject<ProcessEvent>(ee.Data, settings);
                    timeVisual.setTime(pe.MsFullTime - pe.MsComplete);
                } else if (e.Type == EventType.QUESTION_COMPLETE) {
                    timeVisual.setTime(0);
                } else if (e.Type == EventType.QUESTION_CANCEL) {
                    timeVisual.setTime(0);
                }
            };
            //websocket.Connect();
            tooltip.SetToolTip(btnTransmit, "Отключен");
            btnForward.Enabled = false;
            btnRewind.Enabled = false;
            btnPlay.Enabled = false;
        }

        private sealed class TimerVisual: UserControl
        {
            private readonly Brush brush = new SolidBrush(Color.Black);
            private readonly Font font;
            private string drawable = "";


            public TimerVisual()
            {
                this.font = new Font("Lucida Console", 25, FontStyle.Bold);
                setTime(0);
                Size = new Size(200, 30);
            }

            protected override void OnPaint(PaintEventArgs e)
            {
                var graphics = e.Graphics;
                graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
                graphics.DrawString(drawable, font, brush, 0, 0);
                //graphics.DrawRectangle(new Pen(Color.Red, 2), 2, 2, Width - 4, Height - 4);
            }

            public void setTime(long time)
            {
                var timespan = TimeSpan.FromMilliseconds(time);
                drawable = string.Format("{0}:{1}", timespan.Minutes.ToString("D2"), timespan.Seconds.ToString("D2"));
                Invalidate();
            }
        }

        private sealed class ControlButton : UserControl
        {
            private readonly ToolTip tooltip = new ToolTip();
            private readonly Image original;
            private readonly Image pressed;

            private Image drawable;

            public ControlButton(Image original, Image pressed)
            {
                this.original = original;
                this.pressed = pressed;
                drawable = original;
                Size = original.Size;
                MouseEnter += Control_MouseEnter;
                MouseLeave += Control_MouseLeave;
            }

            protected override void OnPaint(PaintEventArgs e)
            {
                var graphics = e.Graphics;
                graphics.DrawImage(drawable, 0, 0);
            }

            private void Control_MouseLeave(object sender, EventArgs e)
            {
                drawable = original;
                Invalidate();
            }

            private void Control_MouseEnter(object sender, EventArgs e)
            {
                drawable = pressed;
                Invalidate();
            }
        }

        private void btnTransmit_Click(object sender, EventArgs e)
        {
            if (!websocket.IsAlive) {
                websocket.Connect();
            } else {
                websocket.Close();
            }
        }
    }
}
