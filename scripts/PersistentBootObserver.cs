// Read-only linked handoff observation for the development regression lane.
// Arm reset-entry hooks only at a genuine terminal BX, after code replacement;
// payload PCs may otherwise coincide with hot routines in the selector.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using Antmicro.Renode.Core;
using Antmicro.Renode.Peripherals;
using Antmicro.Renode.Peripherals.Bus;
using Antmicro.Renode.Peripherals.CPU;

namespace Antmicro.Renode.Peripherals.VCVQuBit.Development
{
    public sealed class PersistentBootObserver : IPeripheral, IBytePeripheral, IKnownSize
    {
        public PersistentBootObserver(IMachine machine, ulong imageBase, int imageSize,
            string imageHash, uint resetVector, ulong finalCheckpoint)
        {
            this.machine = machine;
            this.imageBase = imageBase;
            this.imageSize = imageSize;
            this.imageHash = imageHash;
            this.resetVector = resetVector;
            this.finalCheckpoint = finalCheckpoint;
        }

        public void ObserveBranch(ulong site, int blobSize, string blobHash)
        {
            var cpu = machine.SystemBus.GetCPUs().OfType<ICPUWithHooks>().Single();
            if(!blobs.ContainsKey(site))
            {
                blobs[site] = new List<Tuple<int, string>>();
                cpu.AddHook(site, (source, address) => {
                    if(machine.SystemBus.ReadWord(address) != 0x4730) return; // BX R6
                    Require(blobs[address].Any(blob => Hash(0x38000000, blob.Item1) == blob.Item2),
                        "terminal blob mismatch");
                    Require(++branches == 1, "extra terminal branch");
                    var registers = (ICPUWithRegisters)source;
                    Require(registers.GetRegister("R6").RawValue == resetVector,
                        "unexpected payload reset PC");
                    Require(Hash(imageBase, imageSize) == imageHash, "payload bytes differ");
                    Require(machine.SystemBus.ReadDoubleWord(0xe000ed08) == imageBase,
                        "incorrect payload VTOR");
                    Console.WriteLine("PERSISTENT_HANDOFF_BRANCH");
                    cpu.AddHook(resetVector & ~1UL, (entryCpu, entryPc) => {
                        Require(++entries == 1, "extra payload reset entry");
                        Require(((ICPUWithRegisters)entryCpu).GetRegister("SP").RawValue == 0x20020000,
                            "incorrect initial stack");
                        Require(machine.SystemBus.ReadDoubleWord(0xe000ed08) == imageBase,
                            "incorrect entry VTOR");
                        Require(machine.SystemBus.ReadBytes(0x30000000, 32768).All(value => value == 0),
                            "DMA arena not cleared");
                        Console.WriteLine("PERSISTENT_DMA_ZERO");
                        Console.WriteLine("PERSISTENT_PAYLOAD_RESET");
                        if(finalCheckpoint == (resetVector & ~1UL))
                        {
                            Console.WriteLine("PERSISTENT_PAYLOAD_HALT");
                            entryCpu.IsHalted = true;
                        }
                    });
                    if(finalCheckpoint != (resetVector & ~1UL))
                        cpu.AddHook(finalCheckpoint, (entryCpu, entryPc) => {
                            Require(entries == 1, "final checkpoint before reset entry");
                            Console.WriteLine("PERSISTENT_PAYLOAD_HALT");
                            entryCpu.IsHalted = true;
                        });
                });
            }
            blobs[site].Add(Tuple.Create(blobSize, blobHash));
        }

        public string Diagnostics => string.Format("PERSISTENT_HANDOFF branches={0} entries={1}", branches, entries);
        public void Reset() { branches = entries = 0; }
        public long Size => 0x1000;
        public byte ReadByte(long offset) { throw new InvalidOperationException("observer is not guest storage"); }
        public void WriteByte(long offset, byte value) { throw new InvalidOperationException("observer is not guest storage"); }
        private string Hash(ulong address, int size)
        {
            using(var algorithm = SHA256.Create())
                return BitConverter.ToString(algorithm.ComputeHash(machine.SystemBus.ReadBytes(address, size)))
                    .Replace("-", "").ToLowerInvariant();
        }
        private static void Require(bool condition, string message)
        { if(!condition) throw new InvalidOperationException("persistent boot observer: " + message); }
        private readonly IMachine machine;
        private readonly ulong imageBase, finalCheckpoint;
        private readonly int imageSize;
        private readonly string imageHash;
        private readonly uint resetVector;
        private readonly Dictionary<ulong, List<Tuple<int, string>>> blobs = new Dictionary<ulong, List<Tuple<int, string>>>();
        private int branches, entries;
    }
}
