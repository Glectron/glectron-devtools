using CefSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    public class JavaScriptInjectionFilter : IResponseFilter
    {
        /// <summary>
        /// Location to insert the javascript
        /// </summary>
        public enum Locations
        {
            /// <summary>
            /// Insert Javascript at the top of the header element
            /// </summary>
            head,
            /// <summary>
            /// Insert Javascript at the top of the body element
            /// </summary>
            body
        }

        readonly string injection;
        readonly string location;
        int offset = 0;
        readonly List<byte> overflow = new();

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="injection"></param>
        /// <param name="location"></param>
        public JavaScriptInjectionFilter(string scriptUrl, Locations location = Locations.head)
        {
            injection = "<script src=\"" + scriptUrl + "\"></script>";
            switch (location)
            {
                case Locations.head:
                    this.location = "<head>";
                    break;

                case Locations.body:
                    this.location = "<body>";
                    break;

                default:
                    this.location = "<head>";
                    break;
            }
        }

        /// <summary>
        /// Disposal
        /// </summary>
        public void Dispose()
        {
        }

        /// <summary>
        /// Filter Processing...  handles the injection
        /// </summary>
        /// <param name="dataIn"></param>
        /// <param name="dataInRead"></param>
        /// <param name="dataOut"></param>
        /// <param name="dataOutWritten"></param>
        /// <returns></returns>
        public FilterStatus Filter(Stream dataIn, out long dataInRead, Stream dataOut, out long dataOutWritten)
        {
            dataInRead = dataIn == null ? 0 : dataIn.Length;
            dataOutWritten = 0;

            if (overflow.Count > 0)
            {
                var buffersize = Math.Min(overflow.Count, (int)dataOut.Length);
                dataOut.Write(overflow.ToArray(), 0, buffersize);
                dataOutWritten += buffersize;

                if (buffersize < overflow.Count)
                {
                    overflow.RemoveRange(0, buffersize - 1);
                }
                else
                {
                    overflow.Clear();
                }
            }


            for (var i = 0; i < dataInRead; ++i)
            {
                var readbyte = (byte)dataIn.ReadByte();
                var readchar = Convert.ToChar(readbyte);
                var buffersize = dataOut.Length - dataOutWritten;

                if (buffersize > 0)
                {
                    dataOut.WriteByte(readbyte);
                    dataOutWritten++;
                }
                else
                {
                    overflow.Add(readbyte);
                }

                if (char.ToLower(readchar) == location[offset])
                {
                    offset++;
                    if (offset >= location.Length)
                    {
                        offset = 0;
                        buffersize = Math.Min(injection.Length, dataOut.Length - dataOutWritten);

                        if (buffersize > 0)
                        {
                            var data = Encoding.UTF8.GetBytes(injection);
                            dataOut.Write(data, 0, (int)buffersize);
                            dataOutWritten += buffersize;
                        }

                        if (buffersize < injection.Length)
                        {
                            var remaining = injection.Substring((int)buffersize, (int)(injection.Length - buffersize));
                            overflow.AddRange(Encoding.UTF8.GetBytes(remaining));
                        }

                    }
                }
                else
                {
                    offset = 0;
                }

            }

            if (overflow.Count > 0 || offset > 0)
            {
                return FilterStatus.NeedMoreData;
            }

            return FilterStatus.Done;
        }

        /// <summary>
        /// Initialization
        /// </summary>
        /// <returns></returns>
        public bool InitFilter()
        {
            return true;
        }

    }
}
