enum {
	// This must match the order of CpuStateImpl::Registers in the header
	RA, RX, RY, RD, RSP, REG_MAX
};

namespace {
template<typename U>
struct Addr24bitOp
{
	template<typename T>
	static void Read(WDC65C816 *cpu, T& v)
	{
		cpu->ReadRaw(U::CalcEffectiveAddress(cpu), v);
	}
	template<typename T>
	static void Write(WDC65C816 *cpu, T value)
	{
		cpu->WriteRaw(U::CalcEffectiveAddress(cpu), value);
	}
	template<typename Callable>
	static void Modify(WDC65C816 *cpu, const Callable& fn)
	{
		auto addr = U::CalcEffectiveAddress(cpu);
		typename U::Base::A ret;
		cpu->ReadRaw(addr, ret);
		cpu->WriteRaw(addr, fn(ret));
	}

	template<uint32_t reg>
	struct ReadOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kExecuteSublist, 0, kNoDuplicateList, U::Base::template EffAddr<reg + 1>::ops},
			{JitOperation::kReadNoSegment, JitOperation::kTempReg | reg,
			JitOperation::kTempReg | reg + 1 | JitOperation::Bytes(sizeof(typename U::Base::A))},
			{JitOperation::kEnd},
		};
	};
	template<uint32_t reg>
	struct WriteOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kExecuteSublist, 0, kNoDuplicateList, U::Base::template EffAddr<reg + 1>::ops},
			{JitOperation::kWriteNoSegment, JitOperation::kTempReg | reg,
			JitOperation::kTempReg | reg + 1 | JitOperation::Bytes(sizeof(typename U::Base::A))},
			{JitOperation::kEnd},
		};
	};
};

template<typename U>
struct Addr16bitDataOp
{
	template<typename T>
	static void Read(WDC65C816 *cpu, T& v)
	{
		cpu->ReadDBR(U::CalcEffectiveAddress(cpu), v);
	}
	template<typename T>
	static void Write(WDC65C816 *cpu, T value)
	{
		cpu->WriteDBR(U::CalcEffectiveAddress(cpu), value);
	}
	template<typename Callable>
	static void Modify(WDC65C816 *cpu, const Callable& fn)
	{
		auto addr = U::CalcEffectiveAddress(cpu);
		typename U::Base::A ret;
		cpu->ReadDBR(addr, ret);
		cpu->WriteDBR(addr, fn(ret));
	}

	template<uint32_t reg>
	struct ReadOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kExecuteSublist, 0, kNoDuplicateList, U::Base::template EffAddr<reg + 1>::ops},
			{JitOperation::kRead, JitOperation::kTempReg | reg,
			JitOperation::kTempReg | reg + 1 | JitOperation::Bytes(sizeof(typename U::Base::A))},
			{JitOperation::kEnd},
		};
	};
	template<uint32_t reg>
	struct WriteOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kExecuteSublist, 0, kNoDuplicateList, U::Base::template EffAddr<reg + 1>::ops},
			{JitOperation::kWrite, JitOperation::kTempReg | reg,
			JitOperation::kTempReg | reg + 1 | JitOperation::Bytes(sizeof(typename U::Base::A))},
			{JitOperation::kEnd},
		};
	};
};

template<typename U, bool emulation = false>
struct Addr16bitZeroOp
{
	static void Read(WDC65C816 *cpu, uint8_t& v)
	{
		cpu->ReadZero(U::CalcEffectiveAddress(cpu), v);
	}
	static void Read(WDC65C816 *cpu, uint16_t& v)
	{
		auto addr = U::CalcEffectiveAddress(cpu);
		// The high bit is a magic flag to indicate this access wraps at 256 byte page boundaries
		if (emulation && (addr & 0x80000000)) {
			uint8_t lo, hi;
			cpu->ReadZero(addr, lo);
			cpu->ReadZero((addr & 0xFFFF00) | ((addr + 1) & 0xFF), hi);
			v = ((uint16_t)hi << 8) | lo;
		} else {
			cpu->ReadZero(addr, v);
		}
	}
	template<typename T>
	static void Write(WDC65C816 *cpu, T value)
	{
		cpu->WriteZero(U::CalcEffectiveAddress(cpu), value);
	}
	template<typename Callable>
	static void Modify(WDC65C816 *cpu, const Callable& fn)
	{
		auto addr = U::CalcEffectiveAddress(cpu);
		typename U::Base::A ret;
		cpu->ReadZero(addr, ret);
		cpu->WriteZero(addr, fn(ret));
	}

	template<uint32_t reg>
	struct ReadOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kExecuteSublist, 0, kNoDuplicateList, U::Base::template EffAddr<reg + 1>::ops},
			{JitOperation::kAndImm, JitOperation::kTempReg | reg, 0xFFFF},
			{JitOperation::kReadNoSegment, JitOperation::kTempReg | reg,
			JitOperation::kTempReg | reg + 1 | JitOperation::Bytes(sizeof(typename U::Base::A))},
			{JitOperation::kEnd},
		};
	};
	template<uint32_t reg>
	struct WriteOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kExecuteSublist, 0, kNoDuplicateList, U::Base::template EffAddr<reg + 1>::ops},
			{JitOperation::kAndImm, JitOperation::kTempReg | reg, 0xFFFF},
			{JitOperation::kWriteNoSegment, JitOperation::kTempReg | reg,
			JitOperation::kTempReg | reg + 1 | JitOperation::Bytes(sizeof(typename U::Base::A))},
			{JitOperation::kEnd},
		};
	};
};





template<typename AType, typename XYType>
struct AddrA
{
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "A";

	static constexpr uint8_t op_bits = 4;

	static constexpr uint32_t Bytes()
	{
		return 0;
	}

	template<typename T>
	static void Read(WDC65C816 *cpu, T& v)
	{
		cpu->cpu_state.regs.a.get(v);
	}
	static void Write(WDC65C816 *cpu, A v)
	{
		cpu->cpu_state.regs.a.set(v);
	}
	template<typename Callable>
	static void Modify(WDC65C816 *cpu, const Callable& fn)
	{
		A ret;
		cpu->cpu_state.regs.a.get(ret);
		cpu->cpu_state.regs.a.set(fn(ret));
	}

	template<uint32_t reg>
	struct ReadOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kMove, JitOperation::kTempReg | reg, RA},
			{JitOperation::kEnd},
		};
	};
	template<uint32_t reg>
	struct WriteOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kMove, RA, JitOperation::kTempReg | reg},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s A", mnemonic);
	}
};

template<typename AType, typename XYType, typename EffectiveSize>
struct AddrImm
{
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "#%X";

	static constexpr uint8_t op_bits = 8;

	static constexpr uint32_t Bytes()
	{
		return sizeof(EffectiveSize);
	}
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		return cpu->cpu_state.ip + 1;
	}

	template<typename T>
	static void Read(WDC65C816 *cpu, T& v)
	{
		cpu->ReadPBR(CalcEffectiveAddress(cpu), v);
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kMove, JitOperation::kTempReg | reg, JitOperation::kIpReg},
			{JitOperation::kEnd},
		};
	};

	template<uint32_t reg>
	struct ReadOp {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg | JitOperation::Bytes(sizeof(A))},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s #$%02X", mnemonic, cpu->PeekU8(addr + 1));
	}
};

template<uint32_t offset_reg, typename T, bool check_extra_cycle>
struct AddrAbsBase : public Addr24bitOp<AddrAbsBase<offset_reg, T, check_extra_cycle>>
{
	typedef T Base;
	static constexpr uint32_t Bytes() { return 2; }

	static constexpr bool kMaybeHasConditionalCycle = offset_reg != REG_MAX;

	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint16_t addr;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		uint32_t eff_addr = addr;
		if constexpr(offset_reg != REG_MAX) {
			eff_addr += cpu->cpu_state.regs.reglist[offset_reg].u16;
			if(check_extra_cycle && ((eff_addr ^ addr) & 0xFF00))
				cpu->cpu_state.cycle += cpu->internal_cycle_timing;
		}
		return eff_addr + cpu->cpu_state.data_segment_base;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		const char *index = "";
		if(offset_reg == RX) index = ", X";
		if(offset_reg == RY) index = ", Y";

		sprintf(formatted_str, "%s $%04X%s", mnemonic, cpu->PeekU16(addr + 1), index);
	}
};

template<typename AType, typename XYType>
struct AddrAbs : public AddrAbsBase<REG_MAX, AddrAbs<AType, XYType>, false>
{
	typedef AType A;
	typedef XYType XY;

	static constexpr const char kFormat[] = "%X";

	static constexpr uint8_t op_bits = 0xC;

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(2)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, JitOperation::kDataBase},
			{JitOperation::kEnd},
		};
	};
};

template<typename AType, typename XYType, bool check_extra_cycle = true>
struct AddrAbsX : public AddrAbsBase<RX, AddrAbsX<AType, XYType, check_extra_cycle>, check_extra_cycle>
{
	typedef AType A;
	typedef XYType XY;

	static constexpr const char kFormat[] = "%X,X";

	static constexpr uint8_t op_bits = 0xC;
	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(2)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, JitOperation::kDataBase},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RX},
			{JitOperation::kEnd},
		};
	};
};

template<typename AType, typename XYType, bool check_extra_cycle = true>
struct AddrAbsY : public AddrAbsBase<RY, AddrAbsX<AType, XYType, check_extra_cycle>, check_extra_cycle>
{
	typedef AType A;
	typedef XYType XY;

	static constexpr const char kFormat[] = "%X,Y";

	static constexpr uint8_t op_bits = 0xC;

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(2)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, JitOperation::kDataBase},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RY},
			{JitOperation::kEnd},
		};
	};
};

template<typename AType, typename XYType>
struct AddrAbsLong : Addr24bitOp<AddrAbsLong<AType, XYType>>
{
	typedef AddrAbsLong Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "%X";

	static constexpr uint8_t op_bits = 0x00;

	static constexpr uint32_t Bytes() { return 3; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint16_t addr;
		uint8_t bank;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		cpu->ReadPBR(cpu->cpu_state.ip + 3, bank);
		return addr | ((uint32_t)bank << 16);
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(3)},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		uint32_t v = cpu->PeekU16(addr + 1);
		v |= (uint32_t)cpu->PeekU8(addr + 3) << 16;
		sprintf(formatted_str, "%s $%06X", mnemonic, v);
	}
};

template<typename AType, typename XYType>
struct AddrAbsLongX : Addr24bitOp<AddrAbsLongX<AType, XYType>>
{
	typedef AddrAbsLongX Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "%X,X";

	static constexpr uint8_t op_bits = 0x00;

	static constexpr uint32_t Bytes() { return 3; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint16_t addr;
		uint8_t bank;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		cpu->ReadPBR(cpu->cpu_state.ip + 3, bank);
		return addr + ((uint32_t)bank << 16) + cpu->cpu_state.regs.x.u16;
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(3)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RX},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		uint32_t v = cpu->PeekU16(addr + 1);
		v |= (uint32_t)cpu->PeekU8(addr + 3) << 16;
		sprintf(formatted_str, "%s $%06X, X", mnemonic, v);
	}
};

template<uint32_t offset_reg, typename T, bool emulation = false>
struct AddrDirectBase : public Addr16bitZeroOp<AddrDirectBase<offset_reg, T, emulation>, emulation>
{
	static constexpr bool kMaybeHasConditionalCycle = false;
	typedef T Base;
	static constexpr uint32_t Bytes() { return 1; }

	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		if constexpr(offset_reg != REG_MAX)
			cpu->InternalOp();
		uint8_t addr;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		uint32_t direct = cpu->cpu_state.regs.d.u16;
		if(cpu->cpu_state.regs.d.u8[0]) {
			cpu->cpu_state.cycle += cpu->internal_cycle_timing;
		} else if constexpr(emulation && offset_reg != REG_MAX) {
			// Emulation w/ DL == 0, page wrap
			return 0x80000000 | direct |
				((cpu->cpu_state.regs.reglist[offset_reg].u8[0] + addr) & 0xFF);
		} else if constexpr(emulation) {
			return 0x80000000 | (direct + addr);
		}
		if constexpr(offset_reg != REG_MAX) {
			cpuaddr_t sum = (uint32_t)cpu->cpu_state.regs.reglist[offset_reg].u16 + addr;
			if(sum > 0xFF)
				cpu->cpu_state.cycle += cpu->internal_cycle_timing;
			return direct + sum;
		} else {
			return direct + addr;
		}
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		const char *index = "";
		if(offset_reg == RX) index = ", X";
		if(offset_reg == RY) index = ", Y";

		sprintf(formatted_str, "%s $%02X%s", mnemonic, cpu->PeekU8(addr + 1), index);
	}
};



template<typename AType, typename XYType, bool emulation = false>
struct AddrDirect : AddrDirectBase<REG_MAX, AddrDirect<AType, XYType, emulation>, emulation>
{
	typedef AType A;
	typedef XYType XY;

	static constexpr const char kFormat[] = "%X";

	static constexpr uint8_t op_bits = 0x4;

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kEnd},
		};
	};
};

template<typename AType, typename XYType, bool emulation = false>
struct AddrDirectX : AddrDirectBase<RX, AddrDirectX<AType, XYType, emulation>, emulation>
{
	typedef AType A;
	typedef XYType XY;

	static constexpr const char kFormat[] = "%X,X";

	static constexpr uint8_t op_bits = 0x4;

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RX},
			{JitOperation::kEnd},
		};
	};
};

template<typename AType, typename XYType, bool emulation = false>
struct AddrDirectY : AddrDirectBase<RY, AddrDirectY<AType, XYType, emulation>, emulation>
{
	typedef AType A;
	typedef XYType XY;

	static constexpr const char kFormat[] = "%X,Y";

	static constexpr uint8_t op_bits = 0x4;

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RY},
			{JitOperation::kEnd},
		};
	};
};

template<typename AType, typename XYType>
struct AddrDirectIndirect : Addr24bitOp<AddrDirectIndirect<AType, XYType>>
{
	typedef AddrDirectIndirect Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "(%X)";

	static constexpr uint8_t op_bits = 0x2;

	static constexpr uint32_t Bytes() { return 1; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint8_t addr;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		uint16_t ret;
		cpu->ReadZero(cpu->cpu_state.regs.d.u16 + addr, ret);
		return cpu->cpu_state.data_segment_base + ret;
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, JitOperation::kDataBase},
			{JitOperation::kRead, JitOperation::kTempReg | reg, JitOperation::kTempReg | reg | JitOperation::Bytes(2)},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s ($%02X)", mnemonic, cpu->PeekU8(addr + 1));
	}
};

template<typename AType, typename XYType, bool emulation = false>
struct AddrDirectIndirectX : Addr24bitOp<AddrDirectIndirectX<AType, XYType, emulation>>
{
	typedef AddrDirectIndirectX Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "(%X,X)";

	static constexpr uint8_t op_bits = 0x2;

	static constexpr uint32_t Bytes() { return 1; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint16_t ret;
		AddrDirectX<AType, XYType, emulation>::Read(cpu, ret);
		return cpu->cpu_state.data_segment_base + ret;
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, JitOperation::kDataBase},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RX},
			{JitOperation::kRead, JitOperation::kTempReg | reg, JitOperation::kTempReg | reg | JitOperation::Bytes(2)},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s ($%02X, X)", mnemonic, cpu->PeekU8(addr + 1));
	}
};

template<typename AType, typename XYType, bool check_extra_cycle, bool emulation = false>
struct AddrDirectIndirectY : Addr24bitOp<AddrDirectIndirectY<AType, XYType, check_extra_cycle, emulation>>
{
	typedef AddrDirectIndirectY Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = true;
	static constexpr const char kFormat[] = "(%X),Y";

	static constexpr uint8_t op_bits = 0x2;

	static constexpr uint32_t Bytes() { return 1; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint16_t ret;
		AddrDirect<AType, XYType, emulation>::Read(cpu, ret);
		uint32_t sum = (uint32_t)ret + (uint32_t)cpu->cpu_state.regs.y.u16;
		if(check_extra_cycle && !cpu->mode_long_xy && ((sum ^ ret) & 0x100))
			cpu->cpu_state.cycle += cpu->internal_cycle_timing;
		return cpu->cpu_state.data_segment_base + sum;
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, JitOperation::kDataBase},
			{JitOperation::kRead, JitOperation::kTempReg | reg, JitOperation::kTempReg | reg | JitOperation::Bytes(2)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RY},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s ($%02X), Y", mnemonic, cpu->PeekU8(addr + 1));
	}
};

template<typename AType, typename XYType>
struct AddrDirectIndirectLong : Addr24bitOp<AddrDirectIndirectLong<AType, XYType>>
{
	typedef AddrDirectIndirectLong Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "[%X]";

	static constexpr uint8_t op_bits = 0x41;

	static constexpr uint32_t Bytes() { return 1; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint8_t addr;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		uint16_t effective_addr = cpu->cpu_state.regs.d.u16 + addr;
		uint16_t zp_addr;
		cpu->ReadZero(effective_addr, zp_addr);
		uint8_t zp_bank;
		cpu->ReadZero(effective_addr + 2, zp_bank);
		return zp_addr | ((uint32_t)zp_bank << 16);
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kRead, JitOperation::kTempReg | reg, JitOperation::kTempReg | reg | JitOperation::Bytes(3)},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s [$%02X]", mnemonic, cpu->PeekU8(addr + 1));
	}
};


template<typename AType, typename XYType>
struct AddrDirectIndirectLongY : Addr24bitOp<AddrDirectIndirectLongY<AType, XYType>>
{
	typedef AddrDirectIndirectLongY Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "[%X],Y";

	static constexpr uint8_t op_bits = 0x41;

	static constexpr uint32_t Bytes() { return 1; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint8_t addr;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		uint16_t direct_addr = cpu->cpu_state.regs.d.u16 + addr;
		uint16_t zp_addr;
		cpu->ReadZero(direct_addr, zp_addr);
		uint8_t zp_bank;
		cpu->ReadZero(direct_addr + 2, zp_bank);
		return (zp_addr | ((uint32_t)zp_bank << 16)) + cpu->cpu_state.regs.y.u16;
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kRead, JitOperation::kTempReg | reg, JitOperation::kTempReg | reg | JitOperation::Bytes(3)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RY},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s [$%02X], Y", mnemonic, cpu->PeekU8(addr + 1));
	}
};


template<typename AType, typename XYType>
struct AddrStack : Addr16bitZeroOp<AddrStack<AType, XYType>>
{
	typedef AddrStack Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "%X,S";

	static constexpr uint8_t op_bits = 0x41;

	static constexpr uint32_t Bytes() { return 1; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint8_t addr;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		return cpu->cpu_state.regs.sp.u16 + addr;
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RSP},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s $%02X, S", mnemonic, cpu->PeekU8(addr + 1));
	}
};


template<typename AType, typename XYType>
struct AddrStackY : Addr24bitOp<AddrStackY<AType, XYType>>
{
	typedef AddrStackY Base;
	typedef AType A;
	typedef XYType XY;

	static constexpr bool kMaybeHasConditionalCycle = false;
	static constexpr const char kFormat[] = "(%X,S),Y";

	static constexpr uint8_t op_bits = 0x41;

	static constexpr uint32_t Bytes() { return 1; }
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint8_t addr;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, addr);
		uint16_t pointer_addr = cpu->cpu_state.regs.sp.u16 + addr;
		uint16_t pointer;
		cpu->ReadZero(pointer_addr, pointer);
		return cpu->cpu_state.data_segment_base + pointer + cpu->cpu_state.regs.y.u16;
	}

	template<uint32_t reg>
	struct EffAddr {
		static constexpr JitOperation ops[] = {
			{JitOperation::kReadImm, JitOperation::kTempReg | reg, JitOperation::Bytes(1)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RSP},
			{JitOperation::kRead, JitOperation::kTempReg | reg, JitOperation::kTempReg | reg | JitOperation::Bytes(2)},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, RY},
			{JitOperation::kAdd, JitOperation::kTempReg | reg, JitOperation::kDataBase},
			{JitOperation::kEnd},
		};
	};

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s ($%02X, S), Y", mnemonic, cpu->PeekU8(addr + 1));
	}
};





template<typename AddrMode>
struct OpTsb
{
	typedef typename AddrMode::A A;
	static constexpr uint8_t opcode = 0 | AddrMode::op_bits;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "TSB";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](A mem) {
			A a;
			cpu->cpu_state.regs.a.get(a);
			cpu->cpu_state.zero = a & mem;
			return (A)(mem | a);
		});
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};


template<typename AddrMode>
struct OpTrb
{
	typedef typename AddrMode::A A;
	static constexpr uint8_t opcode = 0 | AddrMode::op_bits;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "TRB";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](A mem) {
			A a;
			cpu->cpu_state.regs.a.get(a);
			cpu->cpu_state.zero = a & mem;
			return (A)(mem & ~a);
		});
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};



template<typename AddrMode>
struct OpBit
{
	typedef typename AddrMode::A A;
	static constexpr uint8_t opcode = 0 | AddrMode::op_bits;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "BIT";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		A mem;
		AddrMode::Read(cpu, mem);
		A a;
		cpu->cpu_state.regs.a.get(a);
		cpu->cpu_state.zero = a & mem;
		if(sizeof(mem) > 1) {
			mem >>= 8;
		}
		cpu->cpu_state.negative = mem >> 7;
		cpu->cpu_state.other_flags = (cpu->cpu_state.other_flags & 0xFFBF) | (mem & 0x40);
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

template<typename AddrMode>
struct OpBitImm
{
	typedef typename AddrMode::A A;
	static constexpr uint8_t opcode = 0 | AddrMode::op_bits;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "BIT";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		A mem;
		AddrMode::Read(cpu, mem);
		A a;
		cpu->cpu_state.regs.a.get(a);
		cpu->cpu_state.zero = a & mem;
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};


template<typename AddrMode, typename Impl>
struct OpAlu
{
	static constexpr uint8_t opcode = Impl::opcode | AddrMode::op_bits;

	typedef AddrMode Addr;
	typedef typename AddrMode::A A;
	typedef typename AddrMode::XY XY;
	static constexpr size_t bA = sizeof(A);
	static constexpr size_t bXY = sizeof(XY);
	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		// Read data into temp reg 0
		{JitOperation::kExecuteSublist, 0, 0, AddrMode::template ReadOp<0>::ops},
		// A ?= temp reg 1
		{Impl::op, RA, JitOperation::kTempReg | 0},
		// Update flags from A
		{JitOperation::kUpdateFlags, kFlagZero | kFlagNegative | bA, RA},
		{JitOperation::kIncrementIP, 0, 1+bA},
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		A mem;
		AddrMode::Read(cpu, mem);
		A a;
		cpu->cpu_state.regs.a.get(a);
		Impl::DoAluOp(a, mem);
		cpu->cpu_state.regs.a.set(a);
		cpu->SetNZ(a);
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

struct OrImpl {
	static constexpr JitOperation::Oper op = JitOperation::kOr;
	static constexpr const char kMnemonic[] = "ORA";
	static constexpr uint8_t opcode = 1;
	template<typename T> static void DoAluOp(T& a, T v) { a |= v; }
};
template<typename T>
using OpOr = OpAlu<T, OrImpl>;

struct AndImpl {
	static constexpr JitOperation::Oper op = JitOperation::kAnd;
	static constexpr const char kMnemonic[] = "AND";
	static constexpr uint8_t opcode = 0x21;
	template<typename T> static void DoAluOp(T& a, T v) { a &= v; }
};
template<typename T>
using OpAnd = OpAlu<T, AndImpl>;

struct XorImpl {
	static constexpr JitOperation::Oper op = JitOperation::kXor;
	static constexpr const char kMnemonic[] = "EOR";
	static constexpr uint8_t opcode = 0x41;
	template<typename T> static void DoAluOp(T& a, T v) { a ^= v; }
};
template<typename T>
using OpXor = OpAlu<T, XorImpl>;

}

// SHIFTS

template<typename AddrMode>
struct OpAsl
{
	static constexpr uint8_t opcode = 2 | AddrMode::op_bits;

	typedef AddrMode Addr;
	typedef typename AddrMode::A A;
	typedef typename AddrMode::XY XY;
	static constexpr size_t bA = sizeof(A);
	static constexpr size_t bXY = sizeof(XY);
	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char kMnemonic[] = "ASL";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		// Read data into temp reg 0
		{JitOperation::kExecuteSublist, 0, 0, AddrMode::template ReadOp<0>::ops},
		// A |= temp reg 1
		{JitOperation::kShiftLeftImm, JitOperation::kTempReg | 0, 1},
		// Update flags from A
		{JitOperation::kUpdateFlags, kFlagCarry | kFlagZero | kFlagNegative | bA, JitOperation::kTempReg | 0},
		{JitOperation::kExecuteSublist, 0, 0, AddrMode::template WriteOp<0>::ops},
		{JitOperation::kIncrementIP, 0, 1+bA},
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](A a) {
			A b = a << 1;
			cpu->SetNZ(b);
			cpu->cpu_state.carry = a >> (8 * bA - 2);
			cpu->InternalOp();
			if constexpr(AddrMode::kMaybeHasConditionalCycle)
				cpu->InternalOp();
			return b;
		});
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

template<typename AddrMode>
struct OpLsr
{
	static constexpr uint8_t opcode = 2 | AddrMode::op_bits;

	typedef AddrMode Addr;
	typedef typename AddrMode::A A;
	typedef typename AddrMode::XY XY;
	static constexpr size_t bA = sizeof(A);
	static constexpr size_t bXY = sizeof(XY);
	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char kMnemonic[] = "LSR";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void SetFlags(WDC65C816 *cpu, A v)
	{
		cpu->cpu_state.zero = v >> 1;
		cpu->cpu_state.negative = 0;
		cpu->cpu_state.carry = (v & 1) << 1;
	}

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](A a) {
			A b = a >> 1;
			SetFlags(cpu, a);
			cpu->InternalOp();
			if constexpr(AddrMode::kMaybeHasConditionalCycle)
				cpu->InternalOp();
			return b;
		});
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

template<typename AddrMode>
struct OpRor
{
	static constexpr uint8_t opcode = 2 | AddrMode::op_bits;

	typedef AddrMode Addr;
	typedef typename AddrMode::A A;
	typedef typename AddrMode::XY XY;
	static constexpr size_t bA = sizeof(A);
	static constexpr size_t bXY = sizeof(XY);
	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char kMnemonic[] = "ROR";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](A a) {
			A b = (a >> 1) | ((cpu->cpu_state.carry & 2) << (8 * bA - 2));
			cpu->SetNZ(b);
			cpu->cpu_state.carry = (a & 1) << 1;
			cpu->InternalOp();
			if constexpr(AddrMode::kMaybeHasConditionalCycle)
				cpu->InternalOp();
			return b;
		});
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

template<typename AddrMode>
struct OpRol
{
	static constexpr uint8_t opcode = 2 | AddrMode::op_bits;

	typedef AddrMode Addr;
	typedef typename AddrMode::A A;
	typedef typename AddrMode::XY XY;
	static constexpr size_t bA = sizeof(A);
	static constexpr size_t bXY = sizeof(XY);
	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char kMnemonic[] = "ROR";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](A a) {
			A b = (a << 1) | ((cpu->cpu_state.carry & 2) >> 1);
			cpu->SetNZ(b);
			cpu->cpu_state.carry = a >> (8 * bA - 2);
			cpu->InternalOp();
			if constexpr(AddrMode::kMaybeHasConditionalCycle)
				cpu->InternalOp();
			return b;
		});
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};


// FLAG OPS

struct CLC
{
	static constexpr uint8_t opcode = 0x18;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "CLC";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kClearFlag, kFlagCarry},
		{JitOperation::kIncrementIP, 0, 1},
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.carry = 0;
		cpu->cpu_state.ip += kBytes;
		cpu->InternalOp();
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};
struct SEC
{
	static constexpr uint8_t opcode = 0x38;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "SEC";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kSetFlag, kFlagCarry},
		{JitOperation::kIncrementIP, 0, 1},
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.carry = 2;
		cpu->cpu_state.ip += kBytes;
		cpu->InternalOp();
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct CLI
{
	static constexpr uint8_t opcode = 0x58;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "CLI";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.interrupts = 1;
		cpu->cpu_state.ip += kBytes;
		cpu->InternalOp();
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};
struct SEI
{
	static constexpr uint8_t opcode = 0x78;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "SEI";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.interrupts = 0;
		cpu->cpu_state.ip += kBytes;
		cpu->InternalOp();
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct CLD
{
	static constexpr uint8_t opcode = 0xD8;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "CLD";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.other_flags &= 0xFFF7;
		cpu->cpu_state.ip += kBytes;
		cpu->InternalOp();
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};
struct SED
{
	static constexpr uint8_t opcode = 0xF8;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "SED";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.other_flags |= 0x8;
		cpu->cpu_state.ip += kBytes;
		cpu->InternalOp();
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct CLV
{
	static constexpr uint8_t opcode = 0xB8;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "CLV";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.other_flags &= 0xFFBF;
		cpu->cpu_state.ip += kBytes;
		cpu->InternalOp();
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

// Status bits N V m x D I Z C
// N, Z, C, I, m, x are broken out

struct REP
{
	static constexpr uint8_t opcode = 0xC2;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "REP";
	static constexpr const char *kParamFormat = "#%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t flags;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, flags);
		cpu->cpu_state.ip += kBytes;
		cpu->cpu_state.other_flags &= ~flags;
		cpu->cpu_state.zero |= flags & 2;
		if(flags & 1)
			cpu->cpu_state.carry = 0;
		if(flags & 4)
			cpu->cpu_state.interrupts = 1;
		if(flags & 0x80)
			cpu->cpu_state.negative = 0;
		if(((flags & 0x10) && !cpu->mode_long_xy) || ((flags & 0x20) && !cpu->mode_long_a)) {
			// Changing mode
			if(flags & 0x10)
				cpu->mode_long_xy = true;
			if(flags & 0x20)
				cpu->mode_long_a = true;
			cpu->OnUpdateMode();
		}
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "REP #$%02X", cpu->PeekU8(addr + 1));
		addr += kBytes;
	}
};
struct SEP
{
	static constexpr uint8_t opcode = 0xE2;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "SEP";
	static constexpr const char *kParamFormat = "#%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t flags;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, flags);
		cpu->cpu_state.ip += kBytes;
		cpu->cpu_state.other_flags |= flags;
		if(flags & 1)
			cpu->cpu_state.carry = 2;
		if(flags & 2)
			cpu->cpu_state.zero = 0;
		if(flags & 4)
			cpu->cpu_state.interrupts = 0;
		if(flags & 0x80)
			cpu->cpu_state.negative = 1;
		if(((flags & 0x10) && cpu->mode_long_xy) || ((flags & 0x20) && cpu->mode_long_a)) {
			// Changing mode
			if(flags & 0x10)
				cpu->mode_long_xy = false;
			if(flags & 0x20)
				cpu->mode_long_a = false;
			cpu->OnUpdateMode();
		}
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "SEP #$%02X", cpu->PeekU8(addr + 1));
		addr += kBytes;
	}
};

// STACK OPS
template<typename Impl, typename Size>
struct OpPush
{
	static constexpr uint8_t opcode = Impl::opcode;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		Size v;
		cpu->cpu_state.regs.reglist[Impl::kReg].get(v);
		cpu->Push(v);
		cpu->InternalOp();
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};
struct PhaImpl {
	static constexpr const char kMnemonic[] = "PHA";
	static constexpr uint8_t opcode = 0x48;
	static constexpr uint32_t kReg = RA;
};
template<typename T>
using PHA = OpPush<PhaImpl, T>;
struct PhdImpl {
	static constexpr const char kMnemonic[] = "PHD";
	static constexpr uint8_t opcode = 0x0B;
	static constexpr uint32_t kReg = RD;
};
using PHD = OpPush<PhdImpl, uint16_t>;
struct PhxImpl {
	static constexpr const char kMnemonic[] = "PHX";
	static constexpr uint8_t opcode = 0xDA;
	static constexpr uint32_t kReg = RX;
};
template<typename T>
using PHX = OpPush<PhxImpl, T>;
struct PhyImpl {
	static constexpr const char kMnemonic[] = "PHY";
	static constexpr uint8_t opcode = 0x5A;
	static constexpr uint32_t kReg = RY;
};
template<typename T>
using PHY = OpPush<PhyImpl, T>;


template<typename Impl, typename Size>
struct OpPop
{
	static constexpr uint8_t opcode = Impl::opcode;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		Size v;
		if constexpr(Impl::kReg == RA) {
			cpu->PopOld(v);
		} else {
			cpu->Pop(v);
		}
		cpu->InternalOp(2);
		cpu->cpu_state.regs.reglist[Impl::kReg].set(v);
		cpu->SetNZ(v);
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};
struct PlaImpl {
	static constexpr const char kMnemonic[] = "PLA";
	static constexpr uint8_t opcode = 0x68;
	static constexpr uint32_t kReg = RA;
};
template<typename T>
using PLA = OpPop<PlaImpl, T>;
struct PldImpl {
	static constexpr const char kMnemonic[] = "PLD";
	static constexpr uint8_t opcode = 0x2B;
	static constexpr uint32_t kReg = RD;
};
using PLD = OpPop<PldImpl, uint16_t>;
struct PlxImpl {
	static constexpr const char kMnemonic[] = "PLX";
	static constexpr uint8_t opcode = 0xFA;
	static constexpr uint32_t kReg = RX;
};
template<typename T>
using PLX = OpPop<PlxImpl, T>;
struct PlyImpl {
	static constexpr const char kMnemonic[] = "PHY";
	static constexpr uint8_t opcode = 0x7A;
	static constexpr uint32_t kReg = RY;
};
template<typename T>
using PLY = OpPop<PlyImpl, T>;

struct PHP
{
	static constexpr uint8_t opcode = 0x8;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "PHP";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->Push(cpu->GetStatusRegister());
		cpu->InternalOp();
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct PLP
{
	static constexpr uint8_t opcode = 0x28;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "PLP";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t v;
		cpu->PopOld(v);
		cpu->InternalOp(2);
		cpu->cpu_state.ip += kBytes;
		cpu->SetStatusRegister(v);
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct PHK
{
	static constexpr uint8_t opcode = 0x4B;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "PHK";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t v = cpu->cpu_state.code_segment_base >> 16;
		cpu->Push(v);
		cpu->InternalOp();
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct PHB
{
	static constexpr uint8_t opcode = 0x8B;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "PHB";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t v = cpu->cpu_state.data_segment_base >> 16;
		cpu->Push(v);
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct PLB
{
	static constexpr uint8_t opcode = 0xAB;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "PLB";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t v;
		cpu->Pop(v);
		cpu->cpu_state.data_segment_base = (uint32_t)v << 16;
		cpu->InternalOp(2);
		cpu->SetNZ(v);
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};


// BRANCHES
template<typename AddrMode, typename Impl, bool is_long_jump>
struct OpJmp
{
	static constexpr uint8_t opcode = Impl::opcode;

	static constexpr size_t kBytes = 3 + (is_long_jump ? 1 : 0);
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		auto addr = AddrMode::CalcEffectiveAddress(cpu);
		if constexpr(Impl::push_pc) {
			if constexpr(is_long_jump) {
				cpu->Push((uint8_t)(cpu->cpu_state.code_segment_base >> 16));
			}
			cpu->Push((uint16_t)(cpu->cpu_state.ip + kBytes - 1));
		}
		if constexpr(is_long_jump) {
			cpu->cpu_state.code_segment_base = addr & 0xFF0000;
		}
		cpu->InternalOp(Impl::kInternalCycles);
		cpu->cpu_state.ip = addr;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};
template<uint8_t op>
struct JmpImpl
{
	static constexpr bool push_pc = false;
	static constexpr bool pop_pc = false;
	static constexpr const char kMnemonic[] = "JMP";
	static constexpr uint8_t opcode = op;
	static constexpr uint32_t kInternalCycles = 0;
};
template<uint8_t op>
struct JsrImpl
{
	static constexpr bool push_pc = true;
	static constexpr bool pop_pc = false;
	static constexpr const char kMnemonic[] = "JSR";
	static constexpr uint8_t opcode = op;
	static constexpr uint32_t kInternalCycles = 1;
};

template<typename Impl>
struct OpRts
{
	static constexpr uint8_t opcode = Impl::opcode;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint16_t v;
		cpu->Pop(v);
		cpu->cpu_state.ip = v + 1;
		if constexpr(Impl::is_long) {
			uint8_t p;
			cpu->Pop(p);
			cpu->cpu_state.code_segment_base = ((uint32_t)p) << 16;
		}
		cpu->InternalOp(Impl::kInternalCycles);
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

template<bool is_long, bool emulation_wrap_addr>
struct JmpAddrModeInd
{
	static constexpr const char kFormat[] = "(%X)";
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint16_t v;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, v);
		uint16_t new_pc;
		if constexpr(emulation_wrap_addr) {
			uint8_t lo, hi;
			cpu->ReadZero(v, lo);
			cpu->ReadZero((v & 0xFF00) | ((v + 1) & 0xFF), hi);
			new_pc = ((uint16_t)hi << 8) | lo;
		} else {
			cpu->ReadZero(v, new_pc);
		}
		uint32_t ret = new_pc;
		if constexpr(is_long) {
			uint8_t bank;
			cpu->ReadZero(v + 2, bank);
			ret += (uint32_t)bank << 16;
		}
		return ret;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s ($%06X)", mnemonic, cpu->PeekU16(addr + 1));
	}
};
struct JmpAddrModeIndAbsX
{
	static constexpr const char kFormat[] = "(%X,X)";
	static cpuaddr_t CalcEffectiveAddress(WDC65C816 *cpu)
	{
		uint16_t v;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, v);
		uint16_t new_pc;
		cpu->ReadPBR(v + cpu->cpu_state.regs.x.u16, new_pc);
		return new_pc;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t addr, char *formatted_str, const char *mnemonic)
	{
		sprintf(formatted_str, "%s ($%06X), X", mnemonic, cpu->PeekU16(addr + 1));
	}
};

struct RtsImpl
{
	static constexpr bool is_long = false;
	static constexpr const char kMnemonic[] = "RTS";
	static constexpr uint8_t opcode = 0x60;
	static constexpr uint32_t kInternalCycles = 3;
};
struct RtlImpl
{
	static constexpr bool is_long = true;
	static constexpr const char kMnemonic[] = "RTL";
	static constexpr uint8_t opcode = 0x6B;
	static constexpr uint32_t kInternalCycles = 2;
};

using JMP_ABS = OpJmp<AddrAbs<uint8_t, uint8_t>, JmpImpl<0x4C>, false>;
using JMP_LONG = OpJmp<AddrAbsLong<uint8_t, uint8_t>, JmpImpl<0x5C>, true>;
using JMP_INDABS = OpJmp<JmpAddrModeInd<false, false>, JmpImpl<0x6C>, false>;
using JMP_INDABS_LEGACY = OpJmp<JmpAddrModeInd<false, true>, JmpImpl<0x6C>, false>;
using JMP_INDABSX = OpJmp<JmpAddrModeIndAbsX, JmpImpl<0x7C>, false>;
using JMP_INDLONG = OpJmp<JmpAddrModeInd<true, false>, JmpImpl<0xDC>, true>;

using JSR_ABS = OpJmp<AddrAbs<uint8_t, uint8_t>, JsrImpl<0x20>, false>;
using JSR_LONG = OpJmp<AddrAbsLong<uint8_t, uint8_t>, JsrImpl<0x22>, true>;
using JSR_INDABSX = OpJmp<JmpAddrModeIndAbsX, JsrImpl<0xFC>, false>;

using RTS = OpRts<RtsImpl>;
using RTL = OpRts<RtlImpl>;

struct RTI
{
	static constexpr uint8_t opcode = 0x40;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "RTI";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t flags, bank;
		uint16_t pc;
		cpu->Pop(flags);
		cpu->SetStatusRegister(flags);

		cpu->Pop(pc);
		cpu->InternalOp(2);
		cpu->cpu_state.ip = pc;
		if(!cpu->mode_emulation) {
			cpu->Pop(bank);
			cpu->cpu_state.code_segment_base = (uint32_t)bank << 16;
		}
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

template<typename Impl>
struct CondBranch
{
	static constexpr uint8_t opcode = Impl::opcode;

	static constexpr size_t kBytes = 2;
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = "%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static uint16_t GetOffset(uint8_t offset)
	{
		if(offset & 0x80)
			return offset | 0xFF00;
		return offset;
	}

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t offset;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, offset);
		if(Impl::DoBranch(cpu)) {
			uint16_t offset16 = GetOffset(offset);
			cpu->InternalOp();
			cpu->cpu_state.ip += offset16;
		}
		cpu->cpu_state.ip += kBytes;
	}
	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		uint8_t offset = cpu->PeekU8(addr + 1);
		uint16_t offset16 = GetOffset(offset);
		uint16_t target = (addr + 2 + offset16) & 0xFFFF;
		sprintf(formatted_str, "%s %02X ($%04X %c)", kMnemonic, offset, target,
			Impl::DoBranch(cpu) ? '+' : '-');
		addr += kBytes;
	}
};

struct BccImpl
{
	static constexpr const char kMnemonic[] = "BCC";
	static constexpr uint8_t opcode = 0x90;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !(cpu->cpu_state.carry & 2);
	}
};
using BCC = CondBranch<BccImpl>;
struct BcsImpl
{
	static constexpr const char kMnemonic[] = "BCS";
	static constexpr uint8_t opcode = 0xB0;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !!(cpu->cpu_state.carry & 2);
	}
};
using BCS = CondBranch<BcsImpl>;
struct BeqImpl
{
	static constexpr const char kMnemonic[] = "BEQ";
	static constexpr uint8_t opcode = 0xF0;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !cpu->cpu_state.zero;
	}
};
using BEQ = CondBranch<BeqImpl>;
struct BmiImpl
{
	static constexpr const char kMnemonic[] = "BMI";
	static constexpr uint8_t opcode = 0x30;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !!(cpu->cpu_state.negative & 1);
	}
};
using BMI = CondBranch<BmiImpl>;
struct BneImpl
{
	static constexpr const char kMnemonic[] = "BNE";
	static constexpr uint8_t opcode = 0xD0;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !!cpu->cpu_state.zero;
	}
};
using BNE = CondBranch<BneImpl>;
struct BplImpl
{
	static constexpr const char kMnemonic[] = "BPL";
	static constexpr uint8_t opcode = 0x10;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !(cpu->cpu_state.negative & 1);
	}
};
using BPL = CondBranch<BplImpl>;
struct BraImpl
{
	static constexpr const char kMnemonic[] = "BRA";
	static constexpr uint8_t opcode = 0x80;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return true;
	}
};
using BRA = CondBranch<BraImpl>;
struct BvcImpl
{
	static constexpr const char kMnemonic[] = "BVC";
	static constexpr uint8_t opcode = 0x50;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !(cpu->cpu_state.other_flags & 0x40);
	}
};
using BVC = CondBranch<BvcImpl>;
struct BvsImpl
{
	static constexpr const char kMnemonic[] = "BCS";
	static constexpr uint8_t opcode = 0x70;
	static bool DoBranch(WDC65C816 *cpu)
	{
		return !!(cpu->cpu_state.other_flags & 0x40);
	}
};
using BVS = CondBranch<BvsImpl>;

struct BRL
{
	static constexpr uint8_t opcode = 0x82;

	static constexpr size_t kBytes = 3;
	static constexpr const char *kMnemonic = "BRL";
	static constexpr const char *kParamFormat = "%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint16_t offset;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, offset);
		cpu->cpu_state.ip += offset + 3;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "BRL $%04X", cpu->PeekU16(addr + 1));
		addr += kBytes;
	}
};

// INC/DEC
template<typename AddrMode>
struct OpIncMem
{
	static constexpr uint8_t opcode = 0;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "INC";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](typename AddrMode::A v) {
			v++;
			cpu->InternalOp();
			if constexpr(AddrMode::kMaybeHasConditionalCycle)
				cpu->InternalOp();
			cpu->SetNZ(v);
			return v;
		});
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

template<typename AddrMode>
struct OpDecMem
{
	static constexpr uint8_t opcode = 0;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "DEC";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		AddrMode::Modify(cpu, [cpu](typename AddrMode::A v) {
			v--;
			cpu->InternalOp();
			if constexpr(AddrMode::kMaybeHasConditionalCycle)
				cpu->InternalOp();
			cpu->SetNZ(v);
			return v;
		});
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

template<typename EffectiveSize, typename Impl>
struct IncDecReg
{
	static constexpr uint8_t opcode = Impl::opcode;
	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		EffectiveSize reg;
		cpu->cpu_state.regs.reglist[Impl::kReg].get(reg);
		reg += Impl::delta;
		cpu->cpu_state.regs.reglist[Impl::kReg].set(reg);
		cpu->InternalOp();
		cpu->SetNZ(reg);
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		if(Impl::kReg == RX || Impl::kReg == RY) *str = kMnemonic;
		else *str = "INC A";
		addr += kBytes;
	}
};
struct INXImpl
{
	static constexpr uint8_t opcode = 0xE8;
	static constexpr const char kMnemonic[] = "INX";
	static constexpr int delta = 1;
	static constexpr uint32_t kReg = RX;
};
template<typename AType, typename XYType>
using INX = IncDecReg<XYType, INXImpl>;

struct INYImpl
{
	static constexpr uint8_t opcode = 0xC8;
	static constexpr const char kMnemonic[] = "INY";
	static constexpr int delta = 1;
	static constexpr uint32_t kReg = RY;
};
template<typename AType, typename XYType>
using INY = IncDecReg<XYType, INYImpl>;

struct INAImpl
{
	static constexpr uint8_t opcode = 0x1A;
	static constexpr const char kMnemonic[] = "INA";
	static constexpr int delta = 1;
	static constexpr uint32_t kReg = RA;
};
template<typename AType, typename XYType>
using INA = IncDecReg<AType, INAImpl>;

struct DEXImpl
{
	static constexpr uint8_t opcode = 0xCA;
	static constexpr const char kMnemonic[] = "DEX";
	static constexpr int delta = -1;
	static constexpr uint32_t kReg = RX;
};
template<typename AType, typename XYType>
using DEX = IncDecReg<XYType, DEXImpl>;

struct DEYImpl
{
	static constexpr uint8_t opcode = 0x88;
	static constexpr const char kMnemonic[] = "DEY";
	static constexpr int delta = -1;
	static constexpr uint32_t kReg = RY;
};
template<typename AType, typename XYType>
using DEY = IncDecReg<XYType, DEYImpl>;

struct DEAImpl
{
	static constexpr uint8_t opcode = 0x3A;
	static constexpr const char kMnemonic[] = "DEA";
	static constexpr int delta = -1;
	static constexpr uint32_t kReg = RA;
};
template<typename AType, typename XYType>
using DEA = IncDecReg<AType, DEAImpl>;


// LOAD/STORE
template<typename AddrMode, typename Impl, typename EffectiveSize>
struct OpLoad
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		EffectiveSize reg;
		AddrMode::Read(cpu, reg);
		cpu->cpu_state.regs.reglist[Impl::kReg].set(reg);
		cpu->SetNZ(reg);
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

struct LdaImpl
{
	static constexpr uint32_t kReg = RA;
	static constexpr const char kMnemonic[] = "LDA";
};
struct LdxImpl
{
	static constexpr uint32_t kReg = RX;
	static constexpr const char kMnemonic[] = "LDX";
};
struct LdyImpl
{
	static constexpr uint32_t kReg = RY;
	static constexpr const char kMnemonic[] = "LDY";
};

template<typename T>
using LDA = OpLoad<T, LdaImpl, typename T::A>;
template<typename T>
using LDX = OpLoad<T, LdxImpl, typename T::XY>;
template<typename T>
using LDY = OpLoad<T, LdyImpl, typename T::XY>;

template<typename AddrMode, typename Impl, typename EffectiveSize>
struct OpStore
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		EffectiveSize reg;
		cpu->cpu_state.regs.reglist[Impl::kReg].get(reg);
		AddrMode::Write(cpu, reg);
		// Stores always execute their conditional cycle
		if constexpr(AddrMode::kMaybeHasConditionalCycle)
			cpu->InternalOp();
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

struct StaImpl
{
	static constexpr uint32_t kReg = RA;
	static constexpr const char kMnemonic[] = "STA";
};
struct StxImpl
{
	static constexpr uint32_t kReg = RX;
	static constexpr const char kMnemonic[] = "STX";
};
struct StyImpl
{
	static constexpr uint32_t kReg = RY;
	static constexpr const char kMnemonic[] = "STY";
};

template<typename T>
using STA = OpStore<T, StaImpl, typename T::A>;
template<typename T>
using STX = OpStore<T, StxImpl, typename T::XY>;
template<typename T>
using STY = OpStore<T, StyImpl, typename T::XY>;

template<typename AddrMode>
struct STZ
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "STZ";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		typename AddrMode::A reg = 0;
		AddrMode::Write(cpu, reg);
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

// ADD/SUB/CMP
template<typename AddrMode>
struct OpAdc
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "ADC";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		typename AddrMode::A v;
		AddrMode::Read(cpu, v);
		if(cpu->supports_decimal && (cpu->cpu_state.other_flags & 0x8))
			AddDec(cpu, v);
		else
			AddBin(cpu, v);
		cpu->cpu_state.ip += kBytes;
	}

	template<typename EffectiveSize>
	static void AddDec(WDC65C816 *cpu, EffectiveSize data)
	{
		EffectiveSize reg;
		cpu->cpu_state.regs.a.get(reg);
		panic();
	}
	template<typename EffectiveSize>
	static void AddBin(WDC65C816 *cpu, EffectiveSize data)
	{
		static constexpr EffectiveSize kHighBit = 1U << (8 * sizeof(EffectiveSize) - 1);
		EffectiveSize reg;
		cpu->cpu_state.regs.a.get(reg);

		uint32_t result = (uint32_t)reg + (uint32_t)data + ((cpu->cpu_state.carry >> 1) & 1);
		uint32_t cn = result >> (8 * sizeof(data) - 1);
		cpu->cpu_state.zero = (EffectiveSize)result;
		cpu->cpu_state.carry = cn;
		cpu->cpu_state.negative = cn;
		cpu->cpu_state.other_flags = (cpu->cpu_state.other_flags & 0xFFBF) |
			(((reg ^ result) & ~(reg ^ data) & kHighBit) ? 0x40 : 0);
		reg = result;
		cpu->cpu_state.regs.a.set(reg);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};
template<typename AddrMode>
struct OpSbc
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "SBC";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		typename AddrMode::A v;
		AddrMode::Read(cpu, v);
		if(cpu->supports_decimal && (cpu->cpu_state.other_flags & 0x8))
			SubDec(cpu, v);
		else
			SubBin(cpu, v);
		cpu->cpu_state.ip += kBytes;
	}

	template<typename EffectiveSize>
	static void SubDec(WDC65C816 *cpu, EffectiveSize data)
	{
		EffectiveSize reg;
		cpu->cpu_state.regs.a.get(reg);
		panic();
	}
	template<typename EffectiveSize>
	static void SubBin(WDC65C816 *cpu, EffectiveSize data)
	{
		static constexpr EffectiveSize kHighBit = 1U << (8 * sizeof(EffectiveSize) - 1);
		EffectiveSize reg;
		cpu->cpu_state.regs.a.get(reg);

		uint32_t result = reg - data - 1 + ((cpu->cpu_state.carry >> 1) & 1);
		uint32_t cn = result >> (8 * sizeof(data) - 1);
		cpu->cpu_state.zero = result;
		cpu->cpu_state.carry = ~cn;
		cpu->cpu_state.negative = cn;
		cpu->cpu_state.other_flags = (cpu->cpu_state.other_flags & 0xFFBF) |
			(((reg ^ result) & (reg ^ data) & kHighBit) ? 0x40 : 0);
		reg = result;
		cpu->cpu_state.regs.a.set(reg);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};
template<typename AddrMode>
struct OpCmp
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "CMP";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		typename AddrMode::A v, reg;
		AddrMode::Read(cpu, v);
		cpu->cpu_state.regs.a.get(reg);
		cpu->cpu_state.ip += kBytes;
		DoCmp(cpu, reg, v);
	}

	template<typename EffectiveSize>
	static void DoCmp(WDC65C816 *cpu, EffectiveSize reg, EffectiveSize data)
	{
		EffectiveSize diff = reg - data;
		cpu->SetNZ(diff);
		cpu->cpu_state.carry = (reg >= data) ? 2 : 0;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};
template<typename AddrMode>
struct OpCpx
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "CPX";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		typename AddrMode::XY v, reg;
		AddrMode::Read(cpu, v);
		cpu->cpu_state.regs.x.get(reg);
		cpu->cpu_state.ip += kBytes;
		OpCmp<AddrMode>::DoCmp(cpu, reg, v);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};
template<typename AddrMode>
struct OpCpy
{
	static constexpr uint8_t opcode = 0;//Impl::opcode;

	static constexpr size_t kBytes = 1 + AddrMode::Bytes();
	static constexpr const char *kMnemonic = "CPY";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		typename AddrMode::XY v, reg;
		AddrMode::Read(cpu, v);
		cpu->cpu_state.regs.y.get(reg);
		cpu->cpu_state.ip += kBytes;
		OpCmp<AddrMode>::DoCmp(cpu, reg, v);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		AddrMode::Disassemble(cpu, addr, formatted_str, kMnemonic);
		addr += kBytes;
	}
};

// REG SWAP
template<bool emulation>
struct TXS
{
	static constexpr uint8_t opcode = 0x9A;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "TXS";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};
	static void Exec(WDC65C816 *cpu)
	{
		if constexpr(emulation) {
			cpu->cpu_state.regs.sp.u16 = 0x100 | cpu->cpu_state.regs.x.u8[0];
		} else {
			cpu->cpu_state.regs.sp.u16 = cpu->cpu_state.regs.x.u16;
		}
		cpu->cpu_state.ip += kBytes;
		cpu->cpu_state.cycle += cpu->internal_cycle_timing;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

template<bool emulation>
struct TCS
{
	static constexpr uint8_t opcode = 0x1B;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = "TCS";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};
	static void Exec(WDC65C816 *cpu)
	{
		if constexpr(emulation) {
			cpu->cpu_state.regs.sp.u16 = 0x100 | cpu->cpu_state.regs.a.u8[0];
		} else {
			cpu->cpu_state.regs.sp.u16 = cpu->cpu_state.regs.a.u16;
		}
		cpu->cpu_state.ip += kBytes;
		cpu->cpu_state.cycle += cpu->internal_cycle_timing;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

template<typename EffectiveSize, typename Impl>
struct MoveReg
{
	static constexpr uint8_t opcode = Impl::opcode;

	static constexpr size_t kBytes = 1;
	static constexpr const char *kMnemonic = Impl::kMnemonic;
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};
	static void Exec(WDC65C816 *cpu)
	{
		EffectiveSize v;
		cpu->cpu_state.regs.reglist[Impl::source].get(v);
		cpu->cpu_state.regs.reglist[Impl::dest].set(v);
		cpu->SetNZ(v);
		cpu->cpu_state.ip += kBytes;
		cpu->cpu_state.cycle += cpu->internal_cycle_timing;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct TAXImpl
{
	static constexpr uint8_t opcode = 0xAA;
	static constexpr const char kMnemonic[] = "TAX";
	static constexpr uint32_t dest = RX;
	static constexpr uint32_t source = RA;
};
template<typename AType, typename XYType>
using TAX = MoveReg<XYType, TAXImpl>;

struct TAYImpl
{
	static constexpr uint8_t opcode = 0xA8;
	static constexpr const char kMnemonic[] = "TAY";
	static constexpr uint32_t dest = RY;
	static constexpr uint32_t source = RA;
};
template<typename AType, typename XYType>
using TAY = MoveReg<XYType, TAYImpl>;

struct TSXImpl
{
	static constexpr uint8_t opcode = 0xBA;
	static constexpr const char kMnemonic[] = "TSX";
	static constexpr uint32_t dest = RX;
	static constexpr uint32_t source = RSP;
};
template<typename AType, typename XYType>
using TSX = MoveReg<XYType, TSXImpl>;

struct TXAImpl
{
	static constexpr uint8_t opcode = 0x8A;
	static constexpr const char kMnemonic[] = "TXA";
	static constexpr uint32_t dest = RA;
	static constexpr uint32_t source = RX;
};
template<typename AType, typename XYType>
using TXA = MoveReg<AType, TXAImpl>;

struct TXYImpl
{
	static constexpr uint8_t opcode = 0x9B;
	static constexpr const char kMnemonic[] = "TXY";
	static constexpr uint32_t dest = RY;
	static constexpr uint32_t source = RX;
};
template<typename AType, typename XYType>
using TXY = MoveReg<XYType, TXYImpl>;

struct TYAImpl
{
	static constexpr uint8_t opcode = 0x98;
	static constexpr const char kMnemonic[] = "TYA";
	static constexpr uint32_t dest = RA;
	static constexpr uint32_t source = RY;
};
template<typename AType, typename XYType>
using TYA = MoveReg<AType, TYAImpl>;

struct TYXImpl
{
	static constexpr uint8_t opcode = 0xBB;
	static constexpr const char kMnemonic[] = "TYX";
	static constexpr uint32_t dest = RX;
	static constexpr uint32_t source = RY;
};
template<typename AType, typename XYType>
using TYX = MoveReg<XYType, TYXImpl>;

struct TCDImpl
{
	static constexpr uint8_t opcode = 0x5B;
	static constexpr const char kMnemonic[] = "TCD";
	static constexpr uint32_t dest = RD;
	static constexpr uint32_t source = RA;
};
template<typename AType, typename XYType>
using TCD = MoveReg<uint16_t, TCDImpl>;

struct TDCImpl
{
	static constexpr uint8_t opcode = 0x7B;
	static constexpr const char kMnemonic[] = "TDC";
	static constexpr uint32_t dest = RA;
	static constexpr uint32_t source = RD;
};
template<typename AType, typename XYType>
using TDC = MoveReg<uint16_t, TDCImpl>;

struct TSCImpl
{
	static constexpr uint8_t opcode = 0x3B;
	static constexpr const char kMnemonic[] = "TSC";
	static constexpr uint32_t dest = RA;
	static constexpr uint32_t source = RSP;
};
template<typename AType, typename XYType>
using TSC = MoveReg<uint16_t, TSCImpl>;

// OTHER CONTROL

struct MVN
{
	static constexpr uint8_t opcode = 0x54;

	static constexpr size_t kBytes = 3;
	static constexpr const char kMnemonic[] = "MVN";
	static constexpr const char *kParamFormat = "%X,%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};
	static void Exec(WDC65C816 *cpu)
	{
		union {
			uint16_t arg;
			uint8_t bytes[2];
		};
		cpu->ReadPBR(cpu->cpu_state.ip + 1, arg);
		if(cpu->BlockMove(bytes[0], bytes[1], 1))
			cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "MVN #$%02X,#$%02X", cpu->PeekU8(addr + 1), cpu->PeekU8(addr + 2));
		addr += kBytes;
	}
};

struct MVP
{
	static constexpr uint8_t opcode = 0x44;

	static constexpr size_t kBytes = 3;
	static constexpr const char kMnemonic[] = "MVP";
	static constexpr const char *kParamFormat = "%X,%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};
	static void Exec(WDC65C816 *cpu)
	{
		union {
			uint16_t arg;
			uint8_t bytes[2];
		};
		cpu->ReadPBR(cpu->cpu_state.ip + 1, arg);
		if(cpu->BlockMove(bytes[0], bytes[1], 0xFFFF))
			cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "MVP #$%02X,#$%02X", cpu->PeekU8(addr + 1), cpu->PeekU8(addr + 2));
		addr += kBytes;
	}
};

struct XBA
{
	static constexpr uint8_t opcode = 0xEB;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "XBA";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.ip += kBytes;
		std::swap(cpu->cpu_state.regs.a.u8[0], cpu->cpu_state.regs.a.u8[1]);
		cpu->SetNZ(cpu->cpu_state.regs.a.u8[0]);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct XCE
{
	static constexpr uint8_t opcode = 0xFB;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "XCE";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.ip += kBytes;
		if(!!(cpu->cpu_state.carry & 2) == cpu->mode_emulation) {
			return;
		}
		cpu->mode_emulation = !cpu->mode_emulation;
		cpu->OnUpdateMode();
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

template<uint32_t n, uint32_t b>
struct OpNOPImpl
{
	static constexpr uint8_t opcode = 0xEA;

	static constexpr size_t kBytes = b;
	static constexpr const char kMnemonic[] = "NOP";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->InternalOp(n);
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

using OpNOP = OpNOPImpl<1, 1>;

struct OpWDM
{
	static constexpr uint8_t opcode = 0x42;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "WDM";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct PEA
{
	static constexpr uint8_t opcode = 0xF4;

	static constexpr size_t kBytes = 3;
	static constexpr const char kMnemonic[] = "PEA";
	static constexpr const char *kParamFormat = "#%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint16_t v;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, v);
		cpu->Push(v);
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "PEA #$%04X", cpu->PeekU16(addr + 1));
		addr += kBytes;
	}
};

struct PEI
{
	static constexpr uint8_t opcode = 0xD4;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "PEI";
	static constexpr const char *kParamFormat = "%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint16_t addr;
		AddrDirect<uint16_t, uint16_t>::Read(cpu, addr);
		cpu->cpu_state.ip += kBytes;
		cpu->Push(addr);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "PEI $%04X", cpu->PeekU16(addr + 1));
		addr += kBytes;
	}
};

struct PER
{
	static constexpr uint8_t opcode = 0x62;

	static constexpr size_t kBytes = 3;
	static constexpr const char kMnemonic[] = "PER";
	static constexpr const char *kParamFormat = "%X";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint16_t v;
		cpu->ReadPBR(cpu->cpu_state.ip + 1, v);
		cpu->cpu_state.ip += kBytes;
		v += cpu->cpu_state.ip;
		cpu->Push(v);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		sprintf(formatted_str, "PER %04X", cpu->PeekU16(addr + 1));
		addr += kBytes;
	}
};

struct WAI
{
	static constexpr uint8_t opcode = 0xCB;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "WAI";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.ip += kBytes;
		cpu->WAI();
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};
struct STP
{
	static constexpr uint8_t opcode = 0xCB;

	static constexpr size_t kBytes = 1;
	static constexpr const char kMnemonic[] = "STP";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.ip += kBytes;
		cpu->STP();
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct BRK
{
	static constexpr uint8_t opcode = 0x00;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "BRK";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.ip += kBytes;
		cpu->DoInterrupt(WDC65C816::BRK);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct COP
{
	static constexpr uint8_t opcode = 0x02;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "COP";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		cpu->cpu_state.ip += kBytes;
		cpu->DoInterrupt(WDC65C816::COP);
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};


namespace n6502 {

template<uint32_t n, uint32_t b = 1>
using NOP = OpNOPImpl<n, b>;

// NMOS illegal opcodes
struct ALR
{
	static constexpr uint8_t opcode = 0x4B;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "ALR";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t v;
		cpu->ReadPBR(cpu->cpu_state.ip, v);
		cpu->cpu_state.regs.a.u8[0] &= v;
		OpLsr<AddrA<uint8_t, uint8_t>>::SetFlags(cpu, cpu->cpu_state.regs.a.u8[0]);
		cpu->cpu_state.regs.a.u8[0] >>= 1;
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

struct ANC
{
	static constexpr uint8_t opcode = 0x0B;

	static constexpr size_t kBytes = 2;
	static constexpr const char kMnemonic[] = "ANC";
	static constexpr const char *kParamFormat = "";
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t v;
		cpu->ReadPBR(cpu->cpu_state.ip, v);
		cpu->cpu_state.regs.a.u8[0] &= v;
		cpu->SetNZ(cpu->cpu_state.regs.a.u8[0]);
		cpu->cpu_state.carry = cpu->cpu_state.negative << 1;
		cpu->cpu_state.ip += kBytes;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

template<typename AddrMode>
struct LAX
{
	static constexpr uint8_t opcode = 0x0B;

	static constexpr size_t kBytes = LDA<AddrMode>::kBytes;
	static constexpr const char kMnemonic[] = "LAX";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		LDA<AddrMode>::Exec(cpu);
		cpu->cpu_state.regs.x.u8[0] = cpu->cpu_state.regs.a.u8[0];
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

template<typename AddrMode>
struct SAX
{
	static constexpr uint8_t opcode = 0x0B;

	static constexpr size_t kBytes = STA<AddrMode>::kBytes;
	static constexpr const char kMnemonic[] = "SAX";
	static constexpr const char *kParamFormat = AddrMode::kFormat;
	static constexpr JitOperation ops[] = {
		{JitOperation::kEnd},
	};

	static void Exec(WDC65C816 *cpu)
	{
		uint8_t old = cpu->cpu_state.regs.a.u8[0];
		cpu->cpu_state.regs.a.u8[0] &= cpu->cpu_state.regs.x.u8[0];
		STA<AddrMode>::Exec(cpu);
		cpu->cpu_state.regs.a.u8[0] = old;
	}

	static void Disassemble(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str)
	{
		*str = kMnemonic;
		addr += kBytes;
	}
};

}
