-- wm8731
-- 
-- This entity implements A/D and D/A capability on the Altera DE2
-- WM8731 Audio Codec. Setup of the codec requires the use of I2C
-- to set parameters located in I2C registers. Setup options can
-- be found in the SCI_REG_ROM and SCI_DAT_ROM. This entity is
-- capable of sampling at 48 kHz with 16 bit samples, one sample
-- for the left channel and one sample for the right channel.
-- 
-- Version 1.0
--
-- Designer: Koushik Roy
-- April 23, 2010

LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
USE IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;

ENTITY wm8731 IS
	PORT
	(	
		LDATA, RDATA	:      IN std_logic_vector(15 downto 0); -- 并行外部数据输入
		clk, Reset, INIT : IN std_logic; 
		INIT_FINISH :				OUT std_logic;
		adc_full :			OUT std_logic;
		data_over :          OUT std_logic; -- 采样同步脉冲
		AUD_MCLK :             OUT std_logic; -- CODEC主时钟输出
		AUD_BCLK :             IN std_logic; -- 数字音频位时钟
		AUD_ADCDAT :			IN std_logic;
		AUD_DACDAT :           OUT std_logic; -- DAC数据线
		AUD_DACLRCK, AUD_ADCLRCK :          IN std_logic; -- DAC数据左/右选择
		I2C_SDAT :             OUT std_logic; -- 串行接口数据线
		I2C_SCLK :             OUT std_logic;  -- 串行接口时钟
		ADCDATA : 				OUT std_logic_vector(31 downto 0)
	);
END wm8731;

ARCHITECTURE Behavorial OF wm8731 IS

-- 定义I2C状态机的状态
TYPE I2C_state is (initialize, start, b0, b1, b2, b3, b4, b5, b6, b7, b_ack, 
					a0, a1, a2, a3, a4, a5, a6, a7, a_ack,
					d0, d1, d2, d3, d4, d5, d6, d7, d_ack,
					b_stop0, b_stop1, b_end);
					
-- 定义DAC状态机的状态
TYPE DAC_state is (initial, sync, shift, refill, reload);

-- 定义ADC状态机的状态
TYPE ADC_state is (adc_initial, adc_sync, adc_sync2, adc_shift, adc_ready);

-- 声明信号变量
signal Bcount : integer range 0 to 31;
signal adc_count : integer range 0 to 32;
signal word_count : integer range 0 to 12;
signal LRDATA : std_logic_vector(31 downto 0); -- 存储L和R数据
signal state, next_state : I2C_state;
signal SCI_ADDR, SCI_WORD1, SCI_WORD2 : std_logic_vector(7 downto 0);

signal i2c_counter : std_logic_vector(9 downto 0);
signal SCLK_int, init_over, sck0, sck1, count_en, word_reset : std_logic;
signal SCLK_inhibit : std_logic;

signal dack0, dack1, bck0, bck1, adck0, adck1, flag, flag1 : std_logic;
signal adc_reg_val : std_logic_vector(31 downto 0);

-- 定义常量
CONSTANT word_limit : integer := 8;

-- 定义ROM类型
type rom_type is array (0 to word_limit) of std_logic_vector(7 downto 0);

-- 定义SCI寄存器ROM内容
constant SCI_REG_ROM : rom_type := (
	x"12", -- Deactivate, R9
	x"01", -- Mute L/R, Load Simul, R0
	x"05", -- Headphone volume - R2
	x"08", -- DAC Unmute, R4
	x"0A", -- DAC Stuff, R5
	x"0C", -- More DAC stuff, R6
	x"0E", -- Format, R7
	x"10", -- Normal Mode, R8
	x"12"  -- Reactivate, R9
);

-- 定义SCI数据ROM内容
constant SCI_DAT_ROM : rom_type := (
	"00000000", -- Deactivate - R9
	"00011111", -- ADC L/R Volume - R0 Old: "00011111"
	"11110101", -- Headphone volume - R2 Old: "11111001" "11110001"
	"11010000", -- Select DAC - R4
	"00000101", -- Turn off de-emphasis,  Off with HPF, unmute; DAC Old: "00010110" - R5
	"01100000", -- Device power on, ADC/DAC power on - R6
	"01000011", -- Master, 16-bits, DSP mode; Old: "00001011" - R7
	"00000000", -- Normal, 8kHz - R8 old : "00001100"
	"00000001" -- Reactivate - R9
);
	
BEGIN
	SCI_ADDR <= "00110100"; -- 设备地址
	
	-- 在DACLRCK的上升沿加载新的L/R通道数据
	-- 减小采样计数器
	DACData_reg : process(Clk, Reset, LDATA, RDATA, dack0, dack1, Bcount, flag1)
	begin
		if (Reset = '1') then
			LRDATA <= CONV_STD_LOGIC_VECTOR(0, 32);
			Bcount <= 31;
		elsif(rising_edge(Clk)) then
			if (dack0 = '1' and dack1 = '0') then -- 上升沿
				LRDATA <= LDATA & RDATA;
				Bcount <= 31;
				flag1 <= '1';
			elsif (bck0 = '1' and bck1 = '0' and flag1 = '1') then
				flag1 <= '0';
			elsif (bck0 = '0' and bck1 = '1') then -- BCLK下降沿
					Bcount <= Bcount - 1;
			end if;
		end if;
	end process;
	
	-- I2C计数器
	I2C_Count : process(Clk, Reset)
	begin
		if (Reset = '1') then
			i2c_counter <= CONV_STD_LOGIC_VECTOR(0, 10);
		elsif (rising_edge(Clk)) then
			i2c_counter <= i2c_counter + '1';
		end if;
	end process;
	
	-- 采样SCLK
	SCLK_sample : process(Clk, Reset, SCLK_int)
	begin
		if (Reset = '1') then
			sck0 <= '0';
			sck1 <= '0';
		elsif(rising_edge(Clk)) then
			sck1 <= sck0;
			sck0 <= SCLK_int;
		end if;
	end process;
	
	-- 采样DALRCK
	DALRCK_sample : process(Clk, Reset, AUD_DACLRCK)
	begin
		if (Reset = '1') then
				dack0 <= '0';
				dack1 <= '0';
		elsif(rising_edge(Clk)) then
			dack1 <= dack0;
			dack0 <= AUD_DACLRCK;
		end if;
	end process;
	
	-- 采样ADCLRCK
	ADCLRCK_sample : process(Clk, Reset, AUD_DACLRCK)
	begin
		if (Reset = '1') then
			adck0 <= '0';
			adck1 <= '0';
		elsif(rising_edge(Clk)) then
			adck1 <= adck0;
			adck0 <= AUD_ADCLRCK;
		end if;
	end process;
	
	-- 采样BCLK
	BCLK_sample : process(Clk, Reset, AUD_BCLK)
	begin
		if (Reset = '1') then
			bck0 <= '0';
			bck1 <= '0';
		elsif(rising_edge(Clk)) then
			bck1 <= bck0;
			bck0 <= AUD_BCLK;
		end if;
	end process;
	
	-- Track number of actual transmitted configuration data frames.
word_counter : process(SCLK_int, Count_EN, Reset, word_reset)
begin
    if (Reset = '1' or word_reset = '1') then
        -- 初始化 word_count
        word_count <= 0;
    elsif(falling_edge(SCLK_int)) then
        if (Count_EN = '1') then
            -- 如果允许计数，增加 word_count
            word_count <= word_count + 1;
        else
            word_count <= word_count;
        end if;
    end if;
end process;

state_machine : process(Clk, Reset)
begin
    if (Reset = '1') then
        -- 初始化状态机
        state <= initialize;
    elsif(rising_edge(Clk)) then
        -- 在上升沿时更新状态
        state <= next_state;
    end if;
end process;

-- 逐步执行 I2C 过程，状态机在 SCLK 下降沿逐步进入下一个状态
next_state_i2c : process(Clk, state, SCLK_int, sck0, sck1, word_count, init)
begin
    word_reset <= '0';
    case state is
        when initialize =>
            if (SCLK_INT = '1') then
                -- 如果 SCLK_INT 为高电平，则进入 start 状态
                next_state <= start;
            else
                next_state <= initialize;
            end if;
        when start =>
            if (sck0 = '0' and sck1 = '1') then
                -- SCK 下降沿时进入 b0 状态，开始条件
                next_state <= b0;
            else
                next_state <= start;
            end if;
        -- 其他状态依次类推...
        when b0 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b1;
				else
					next_state <= b0;
				end if;
			when b1 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b2;
				else
					next_state <= b1;
				end if;
			when b2 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b3;
				else
					next_state <= b2;
				end if;
			when b3 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b4;
				else
					next_state <= b3;
				end if;
			when b4 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b5;
				else
					next_state <= b4;
				end if;
			when b5 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b6;
				else
					next_state <= b5;
				end if;
			when b6 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b7;
				else
					next_state <= b6;
				end if;
			when b7 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= b_ack;
				else
					next_state <= b7;
				end if;
			when b_ack =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a0; -- First ack.
				else
					next_state <= b_ack;
				end if;
			when a0 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a1;
				else
					next_state <= a0;
				end if;
			when a1 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a2;
				else
					next_state <= a1;
				end if;
			when a2 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a3;
				else
					next_state <= a2;
				end if;
			when a3 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a4;
				else
					next_state <= a3;
				end if;
			when a4 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a5;
				else
					next_state <= a4;
				end if;
			when a5 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a6;
				else
					next_state <= a5;
				end if;
			when a6 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a7;
				else
					next_state <= a6;
				end if;
			when a7 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= a_ack;
				else
					next_state <= a7;
				end if;
			when a_ack =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d0; -- Second ack
				else
					next_state <= a_ack;
				end if;
			when d0 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d1;
				else
					next_state <= d0;
				end if;
			when d1 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d2;
				else
					next_state <= d1;
				end if;
			when d2 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d3;
				else
					next_state <= d2;
				end if;
			when d3 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d4;
				else
					next_state <= d3;
				end if;
			when d4 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d5;
				else
					next_state <= d4;
				end if;
			when d5 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d6;
				else
					next_state <= d5;
				end if;
			when d6 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d7;
				else
					next_state <= d6;
				end if;
			when d7 =>
				if (sck0 = '0' and sck1 = '1') then
					next_state <= d_ack; -- Last ack
				else
					next_state <= d7;
				end if;
			when d_ack =>
				-- Check to see if we've transmitted the correct number
				-- of words. If so, done. If not, transmit more.
				if (sck0 = '0' and sck1 = '1') then
					if (word_count = word_limit+1) then
						next_state <= b_stop0;
					else
						next_state <= initialize;
					end if;
				else
					next_state <= d_ack;
				end if;
			when b_stop0 =>
				-- If we're done, generate a stop condition
				if (SCLK_INT = '1') then
					next_state <= b_stop1;
				else
					next_state <= b_stop0;
				end if;
			when b_stop1 =>
				next_state <= b_end;
            when b_end =>
                next_state <= b_end;
                word_reset <= '1';  -- 在 b_end 状态时重置 word_count
    end case;
end process;

outputs_i2c : process(state, SCI_ADDR, word_count)
begin
    init_over <= '0';-- 初始化完成信号，初始为低电平
    count_en <= '0'; -- 计数使能信号，初始为低电平
    sclk_inhibit <= '0'; -- SCLK抑制信号，初始为低电平
    case state is-- 状态机开始
        when initialize =>
            I2C_SDAT <= '1'; -- SDAT 起始高电平
        when start =>
            I2C_SDAT <= '0'; -- SDAT 下降沿
        -- 其他状态依次类推...
        when b0 =>
				I2C_SDAT <= SCI_ADDR(7);
			when b1 =>
				I2C_SDAT <= SCI_ADDR(6);
			when b2 =>
				I2C_SDAT <= SCI_ADDR(5);
			when b3 =>
				I2C_SDAT <= SCI_ADDR(4);
			when b4 =>
				I2C_SDAT <= SCI_ADDR(3);
			when b5 =>
				I2C_SDAT <= SCI_ADDR(2);
			when b6 =>
				I2C_SDAT <= SCI_ADDR(1);
			when b7 =>
				I2C_SDAT <= SCI_ADDR(0);
			when b_ack =>
				I2C_SDAT <= 'Z';
			when a0 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(7);
			when a1 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(6);
			when a2 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(5);
			when a3 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(4);
			when a4 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(3);
			when a5 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(2);
			when a6 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(1);
			when a7 =>
				I2C_SDAT <= SCI_REG_ROM(word_count)(0);
			when a_ack =>
				I2C_SDAT <= 'Z';
			when d0 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(7);
			when d1 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(6);
			when d2 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(5);
			when d3 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(4);
			when d4 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(3);
			when d5 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(2);
			when d6 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(1);
			when d7 =>
				I2C_SDAT <= SCI_DAT_ROM(word_count)(0);
        when d_ack =>
            I2C_SDAT <= 'Z';
            count_en <= '1';  -- 在 d_ack 状态时允许计数
        when b_stop0 =>
            -- 保持 SDAT 低电平直到 SCLK 变高
            I2C_SDAT <= '0';
        when b_stop1 =>
            -- SCLK 为高电平时，SDAT 上升沿
            I2C_SDAT <= '1';
            sclk_inhibit <= '1';
        when b_end =>
            I2C_SDAT <= '1';
            init_over <= '1';
            sclk_inhibit <= '1';
    end case;
end process;

	adc_proc : process(Clk, bck0, bck1, adc_reg_val, adck0, adck1, Reset, adc_count, flag)
	begin
		if (Reset = '1') then                   -- 复位信号为高电平时
			adc_reg_val <= CONV_STD_LOGIC_VECTOR(0, 32);  -- 将ADC寄存器的值初始化为全零
			adc_count <= 31;                              -- ADC计数器初始化为31
			adc_full <= '0';                               -- ADC缓冲区满信号置低
		elsif(rising_edge(Clk)) then                     -- 当时钟上升沿触发时
			adc_reg_val(adc_count) <= AUD_ADCDAT;         -- 将AUD_ADCDAT的值存入ADC寄存器中
			adc_full <= '0';                             -- ADC缓冲区满信号置低
			if (adc_count = 0) then                       -- 如果ADC计数器为0
				adc_full <= '1';                           -- ADC缓冲区满信号置高
			end if;
			
			if (adck0 = '1' and adck1 = '0') then         -- 当adck0为高电平且adck1为低电平时（上升沿）
				adc_count <= 31;                            -- 重新开始读取新的ADC数据
			elsif ( (not (adc_count = 0)) and bck0 = '0' and bck1 = '1') then  -- 当ADC计数器不为0且bck0为低电平且bck1为高电平时（下降沿）
					adc_count <= adc_count - 1;              -- 当没有提供新数据时，右移一个新位
			end if;
			
		end if;
	end process;
			
	
	SCLK_int <= I2C_counter(9) or SCLK_inhibit;            -- SCLK（串行时钟）等于I2C计数器最高位或SCLK抑制信号
	--SCLK_int <= I2C_counter(3) or SCLK_inhibit;           -- 仅用于仿真
	AUD_MCLK <= I2C_counter(2);                            -- MCLK（主时钟）等于I2C计数器第2位
	I2C_SCLK <= SCLK_int;                                  -- I2C时钟信号等于SCLK_int

	AUD_DACDAT <= LRDATA(Bcount);                           -- 将LRDATA的值赋给AUD_DACDAT
	data_over <= flag1;                                     -- 数据结束信号等于flag1
	init_finish <= init_over;                               -- 初始化完成信号等于初始化完成标志
	ADCDATA <= adc_reg_val;                                 -- 将ADC寄存器的值赋给ADCDATA
			
end Behavorial;