/*
* jCryption JavaScript data encryption v1.2
* http://www.jcryption.org/
*
* Copyright (c) 2010 Daniel Griesser
* Dual licensed under the MIT and GPL licenses.
* http://www.opensource.org/licenses/mit-license.php
* http://www.opensource.org/licenses/gpl-2.0.php
*
* If you need any further information about this plugin please
* visit my homepage or contact me under daniel.griesser@jcryption.org
*/
(function($) {
	$.jCryption = function(el, options) {
		var base = this;

		base.$el = $(el);
		base.el = el;

		base.$el.data("jCryption", base);
		base.init = function() {

			base.options = $.extend({},$.jCryption.defaultOptions, options);

			$encryptedElement = $("<input />",{
				type:'hidden',
				name:base.options.postVariable
			});

			if (base.options.submitElement !== false) {
				var $submitElement = base.options.submitElement;
			} else {
				var $submitElement = base.$el.find(":input:submit");
			}

			$submitElement.bind(base.options.submitEvent,function() {
				$(this).attr("disabled",true);
				if(base.options.beforeEncryption()) {
					$.jCryption.getKeys(base.options.getKeysURL, function(keys) {
						$.jCryption.encrypt(base.$el.serialize(), keys,function(encrypted) {
							$encryptedElement.val(encrypted);
							$(base.$el).find(base.options.formFieldSelector).attr("disabled",true).end().append($encryptedElement).submit();
						});
					});
				}
				return false;
			});

		};

		base.init();
	};

	$.jCryption.getKeys = function(url,callback) {
		var jCryptionKeyPair = function(encryptionExponent, modulus, maxdigits) {
			setMaxDigits(parseInt(maxdigits,10));
			this.e = biFromHex(encryptionExponent);
			this.m = biFromHex(modulus);
			this.chunkSize = 2 * biHighIndex(this.m);
			this.radix = 16;
			this.barrett = new BarrettMu(this.m);
		};

		$.getJSON(url,function(data){
			var keys = new jCryptionKeyPair(data.e,data.n,data.maxdigits);
			if($.isFunction(callback)) {
				callback.call(this, keys);
			}
		});
	};

	$.jCryption.encrypt = function(string,keyPair,callback) {
		var charSum = 0;
		for(var i = 0; i < string.length; i++){
			charSum += string.charCodeAt(i);
		}
		var tag = '0123456789abcdef';
		var hex = '';
		hex += tag.charAt((charSum & 0xF0) >> 4) + tag.charAt(charSum & 0x0F);

		var taggedString = hex + string;

		var encrypt = [];
		var j = 0;

		while (j < taggedString.length) {
			encrypt[j] = taggedString.charCodeAt(j);
			j++;
		}

		while (encrypt.length % keyPair.chunkSize !== 0) {
			encrypt[j++] = 0;
		}

		function encryption(encryptObject) {
			var charCounter = 0;
			var j, block;
			var encrypted = "";
			function encryptChar() {
				block = new BigInt();
				j = 0;
				for (var k = charCounter; k < charCounter+keyPair.chunkSize; ++j) {
					block.digits[j] = encryptObject[k++];
					block.digits[j] += encryptObject[k++] << 8;
				}
				var crypt = keyPair.barrett.powMod(block, keyPair.e);
				var text = keyPair.radix == 16 ? biToHex(crypt) : biToString(crypt, keyPair.radix);
				encrypted += text + " ";
				charCounter += keyPair.chunkSize;
				if (charCounter < encryptObject.length) {
					setTimeout(encryptChar, 1)
				} else {
					var encryptedString = encrypted.substring(0, encrypted.length - 1);
					if($.isFunction(callback)) {
						callback(encryptedString);
					} else {
						return encryptedString;
					}

				}
			}
			setTimeout(encryptChar, 1);
		}

		encryption(encrypt);
	};

	$.jCryption.defaultOptions = {
		submitElement:false,
		submitEvent:"click",
		getKeysURL:"login.php?generateKeypair=true",
		beforeEncryption:function(){return true},
		postVariable:"jCryption",
		formFieldSelector:":input"
	};

	$.fn.jCryption = function(options) {
		return this.each(function(){
			(new $.jCryption(this, options));
		});
	};

})(jQuery);

/*
* BigInt, a suite of routines for performing multiple-precision arithmetic in
* JavaScript.
* BarrettMu, a class for performing Barrett modular reduction computations in
* JavaScript.
*
*
* Copyright 1998-2005 David Shapiro.
* dave@ohdave.com
*
* changed and improved by Daniel Griesser
* http://www.jcryption.org/
* Daniel Griesser <daniel.griesser@jcryption.org>
*/
var biRadixBase = 2;
var biRadixBits = 16;
var bitsPerDigit = biRadixBits;
var biRadix = 1 << 16;
var biHalfRadix = biRadix >>> 1;
var biRadixSquared = biRadix * biRadix;
var maxDigitVal = biRadix - 1;
var maxInteger = 9999999999999998;
var maxDigits;
var ZERO_ARRAY;
var bigZero, bigOne;
var dpl10 = 15;
var highBitMasks = new Array(0x0000, 0x8000, 0xC000, 0xE000, 0xF000, 0xF800,
0xFC00, 0xFE00, 0xFF00, 0xFF80, 0xFFC0, 0xFFE0,
0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF);

var hexatrigesimalToChar = new Array(
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
'u', 'v', 'w', 'x', 'y', 'z'
);

var hexToChar = new Array('0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
'a', 'b', 'c', 'd', 'e', 'f');

var lowBitMasks = new Array(0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F,
0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF,
0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF);

function setMaxDigits(value) {
	maxDigits = value;
	ZERO_ARRAY = new Array(maxDigits);
	for (var iza = 0; iza < ZERO_ARRAY.length; iza++) ZERO_ARRAY[iza] = 0;
	bigZero = new BigInt();
	bigOne = new BigInt();
	bigOne.digits[0] = 1;
}

function BigInt(flag) {
	if (typeof flag == "boolean" && flag == true) {
		this.digits = null;
	}
	else {
		this.digits = ZERO_ARRAY.slice(0);
	}
	this.isNeg = false;
}

function biFromDecimal(s) {
	var isNeg = s.charAt(0) == '-';
	var i = isNeg ? 1 : 0;
	var result;
	while (i < s.length && s.charAt(i) == '0') ++i;
	if (i == s.length) {
		result = new BigInt();
	}
	else {
		var digitCount = s.length - i;
		var fgl = digitCount % dpl10;
		if (fgl == 0) fgl = dpl10;
		result = biFromNumber(Number(s.substr(i, fgl)));
		i += fgl;
		while (i < s.length) {
			result = biAdd(biMultiply(result, biFromNumber(1000000000000000)),
			biFromNumber(Number(s.substr(i, dpl10))));
			i += dpl10;
		}
		result.isNeg = isNeg;
	}
	return result;
}

function biCopy(bi) {
	var result = new BigInt(true);
	result.digits = bi.digits.slice(0);
	result.isNeg = bi.isNeg;
	return result;
}

function biFromNumber(i) {
	var result = new BigInt();
	result.isNeg = i < 0;
	i = Math.abs(i);
	var j = 0;
	while (i > 0) {
		result.digits[j++] = i & maxDigitVal;
		i >>= biRadixBits;
	}
	return result;
}

function reverseStr(s) {
	var result = "";
	for (var i = s.length - 1; i > -1; --i) {
		result += s.charAt(i);
	}
	return result;
}

function biToString(x, radix) {
	var b = new BigInt();
	b.digits[0] = radix;
	var qr = biDivideModulo(x, b);
	var result = hexatrigesimalToChar[qr[1].digits[0]];
	while (biCompare(qr[0], bigZero) == 1) {
		qr = biDivideModulo(qr[0], b);
		digit = qr[1].digits[0];
		result += hexatrigesimalToChar[qr[1].digits[0]];
	}
	return (x.isNeg ? "-" : "") + reverseStr(result);
}

function biToDecimal(x) {
	var b = new BigInt();
	b.digits[0] = 10;
	var qr = biDivideModulo(x, b);
	var result = String(qr[1].digits[0]);
	while (biCompare(qr[0], bigZero) == 1) {
		qr = biDivideModulo(qr[0], b);
		result += String(qr[1].digits[0]);
	}
	return (x.isNeg ? "-" : "") + reverseStr(result);
}

function digitToHex(n) {
	var mask = 0xf;
	var result = "";
	for (i = 0; i < 4; ++i) {
		result += hexToChar[n & mask];
		n >>>= 4;
	}
	return reverseStr(result);
}

function biToHex(x) {
	var result = "";
	var n = biHighIndex(x);
	for (var i = biHighIndex(x); i > -1; --i) {
		result += digitToHex(x.digits[i]);
	}
	return result;
}

function charToHex(c) {
	var ZERO = 48;
	var NINE = ZERO + 9;
	var littleA = 97;
	var littleZ = littleA + 25;
	var bigA = 65;
	var bigZ = 65 + 25;
	var result;

	if (c >= ZERO && c <= NINE) {
		result = c - ZERO;
	} else if (c >= bigA && c <= bigZ) {
		result = 10 + c - bigA;
	} else if (c >= littleA && c <= littleZ) {
		result = 10 + c - littleA;
	} else {
		result = 0;
	}
	return result;
}

function hexToDigit(s) {
	var result = 0;
	var sl = Math.min(s.length, 4);
	for (var i = 0; i < sl; ++i) {
		result <<= 4;
		result |= charToHex(s.charCodeAt(i))
	}
	return result;
}

function biFromHex(s) {
	var result = new BigInt();
	var sl = s.length;
	for (var i = sl, j = 0; i > 0; i -= 4, ++j) {
		result.digits[j] = hexToDigit(s.substr(Math.max(i - 4, 0), Math.min(i, 4)));
	}
	return result;
}

function biFromString(s, radix) {
	var isNeg = s.charAt(0) == '-';
	var istop = isNeg ? 1 : 0;
	var result = new BigInt();
	var place = new BigInt();
	place.digits[0] = 1; // radix^0
	for (var i = s.length - 1; i >= istop; i--) {
		var c = s.charCodeAt(i);
		var digit = charToHex(c);
		var biDigit = biMultiplyDigit(place, digit);
		result = biAdd(result, biDigit);
		place = biMultiplyDigit(place, radix);
	}
	result.isNeg = isNeg;
	return result;
}

function biDump(b) {
	return (b.isNeg ? "-" : "") + b.digits.join(" ");
}

function biAdd(x, y) {
	var result;

	if (x.isNeg != y.isNeg) {
		y.isNeg = !y.isNeg;
		result = biSubtract(x, y);
		y.isNeg = !y.isNeg;
	}
	else {
		result = new BigInt();
		var c = 0;
		var n;
		for (var i = 0; i < x.digits.length; ++i) {
			n = x.digits[i] + y.digits[i] + c;
			result.digits[i] = n & 0xffff;
			c = Number(n >= biRadix);
		}
		result.isNeg = x.isNeg;
	}
	return result;
}

function biSubtract(x, y) {
	var result;
	if (x.isNeg != y.isNeg) {
		y.isNeg = !y.isNeg;
		result = biAdd(x, y);
		y.isNeg = !y.isNeg;
	} else {
		result = new BigInt();
		var n, c;
		c = 0;
		for (var i = 0; i < x.digits.length; ++i) {
			n = x.digits[i] - y.digits[i] + c;
			result.digits[i] = n & 0xffff;
			if (result.digits[i] < 0) result.digits[i] += biRadix;
			c = 0 - Number(n < 0);
		}
		if (c == -1) {
			c = 0;
			for (var i = 0; i < x.digits.length; ++i) {
				n = 0 - result.digits[i] + c;
				result.digits[i] = n & 0xffff;
				if (result.digits[i] < 0) result.digits[i] += biRadix;
				c = 0 - Number(n < 0);
			}
			result.isNeg = !x.isNeg;
		} else {
			result.isNeg = x.isNeg;
		}
	}
	return result;
}

function biHighIndex(x) {
	var result = x.digits.length - 1;
	while (result > 0 && x.digits[result] == 0) --result;
	return result;
}

function biNumBits(x) {
	var n = biHighIndex(x);
	var d = x.digits[n];
	var m = (n + 1) * bitsPerDigit;
	var result;
	for (result = m; result > m - bitsPerDigit; --result) {
		if ((d & 0x8000) != 0) break;
		d <<= 1;
	}
	return result;
}

function biMultiply(x, y) {
	var result = new BigInt();
	var c;
	var n = biHighIndex(x);
	var t = biHighIndex(y);
	var u, uv, k;

	for (var i = 0; i <= t; ++i) {
		c = 0;
		k = i;
		for (j = 0; j <= n; ++j, ++k) {
			uv = result.digits[k] + x.digits[j] * y.digits[i] + c;
			result.digits[k] = uv & maxDigitVal;
			c = uv >>> biRadixBits;
		}
		result.digits[i + n + 1] = c;
	}
	result.isNeg = x.isNeg != y.isNeg;
	return result;
}

function biMultiplyDigit(x, y) {
	var n, c, uv;
	var result = new BigInt();
	n = biHighIndex(x);
	c = 0;
	for (var j = 0; j <= n; ++j) {
		uv = result.digits[j] + x.digits[j] * y + c;
		result.digits[j] = uv & maxDigitVal;
		c = uv >>> biRadixBits;
	}
	result.digits[1 + n] = c;
	return result;
}

function arrayCopy(src, srcStart, dest, destStart, n) {
	var m = Math.min(srcStart + n, src.length);
	for (var i = srcStart, j = destStart; i < m; ++i, ++j) {
		dest[j] = src[i];
	}
}



function biShiftLeft(x, n) {
	var digitCount = Math.floor(n / bitsPerDigit);
	var result = new BigInt();
	arrayCopy(x.digits, 0, result.digits, digitCount,result.digits.length - digitCount);
	var bits = n % bitsPerDigit;
	var rightBits = bitsPerDigit - bits;
	for (var i = result.digits.length - 1, i1 = i - 1; i > 0; --i, --i1) {
		result.digits[i] = ((result.digits[i] << bits) & maxDigitVal) |
		((result.digits[i1] & highBitMasks[bits]) >>>
		(rightBits));
	}
	result.digits[0] = ((result.digits[i] << bits) & maxDigitVal);
	result.isNeg = x.isNeg;
	return result;
}

function biShiftRight(x, n) {
	var digitCount = Math.floor(n / bitsPerDigit);
	var result = new BigInt();
	arrayCopy(x.digits, digitCount, result.digits, 0,x.digits.length - digitCount);
	var bits = n % bitsPerDigit;
	var leftBits = bitsPerDigit - bits;
	for (var i = 0, i1 = i + 1; i < result.digits.length - 1; ++i, ++i1) {
		result.digits[i] = (result.digits[i] >>> bits) |
		((result.digits[i1] & lowBitMasks[bits]) << leftBits);
	}
	result.digits[result.digits.length - 1] >>>= bits;
	result.isNeg = x.isNeg;
	return result;
}

function biMultiplyByRadixPower(x, n) {
	var result = new BigInt();
	arrayCopy(x.digits, 0, result.digits, n, result.digits.length - n);
	return result;
}

function biDivideByRadixPower(x, n)
{
	var result = new BigInt();
	arrayCopy(x.digits, n, result.digits, 0, result.digits.length - n);
	return result;
}

function biModuloByRadixPower(x, n)
{
	var result = new BigInt();
	arrayCopy(x.digits, 0, result.digits, 0, n);
	return result;
}

function biCompare(x, y) {
	if (x.isNeg != y.isNeg) {
		return 1 - 2 * Number(x.isNeg);
	}
	for (var i = x.digits.length - 1; i >= 0; --i) {
		if (x.digits[i] != y.digits[i]) {
			if (x.isNeg) {
				return 1 - 2 * Number(x.digits[i] > y.digits[i]);
			} else {
				return 1 - 2 * Number(x.digits[i] < y.digits[i]);
			}
		}
	}
	return 0;
}

function biDivideModulo(x, y) {
	var nb = biNumBits(x);
	var tb = biNumBits(y);
	var origYIsNeg = y.isNeg;
	var q, r;
	if (nb < tb) {
		if (x.isNeg) {
			q = biCopy(bigOne);
			q.isNeg = !y.isNeg;
			x.isNeg = false;
			y.isNeg = false;
			r = biSubtract(y, x);
			x.isNeg = true;
			y.isNeg = origYIsNeg;
		} else {
			q = new BigInt();
			r = biCopy(x);
		}
		return new Array(q, r);
	}

	q = new BigInt();
	r = x;

	var t = Math.ceil(tb / bitsPerDigit) - 1;
	var lambda = 0;
	while (y.digits[t] < biHalfRadix) {
		y = biShiftLeft(y, 1);
		++lambda;
		++tb;
		t = Math.ceil(tb / bitsPerDigit) - 1;
	}

	r = biShiftLeft(r, lambda);
	nb += lambda;
	var n = Math.ceil(nb / bitsPerDigit) - 1;

	var b = biMultiplyByRadixPower(y, n - t);
	while (biCompare(r, b) != -1) {
		++q.digits[n - t];
		r = biSubtract(r, b);
	}
	for (var i = n; i > t; --i) {
		var ri = (i >= r.digits.length) ? 0 : r.digits[i];
		var ri1 = (i - 1 >= r.digits.length) ? 0 : r.digits[i - 1];
		var ri2 = (i - 2 >= r.digits.length) ? 0 : r.digits[i - 2];
		var yt = (t >= y.digits.length) ? 0 : y.digits[t];
		var yt1 = (t - 1 >= y.digits.length) ? 0 : y.digits[t - 1];
		if (ri == yt) {
			q.digits[i - t - 1] = maxDigitVal;
		} else {
			q.digits[i - t - 1] = Math.floor((ri * biRadix + ri1) / yt);
		}

		var c1 = q.digits[i - t - 1] * ((yt * biRadix) + yt1);
		var c2 = (ri * biRadixSquared) + ((ri1 * biRadix) + ri2);
		while (c1 > c2) {
			--q.digits[i - t - 1];
			c1 = q.digits[i - t - 1] * ((yt * biRadix) | yt1);
			c2 = (ri * biRadix * biRadix) + ((ri1 * biRadix) + ri2);
		}

		b = biMultiplyByRadixPower(y, i - t - 1);
		r = biSubtract(r, biMultiplyDigit(b, q.digits[i - t - 1]));
		if (r.isNeg) {
			r = biAdd(r, b);
			--q.digits[i - t - 1];
		}
	}
	r = biShiftRight(r, lambda);

	q.isNeg = x.isNeg != origYIsNeg;
	if (x.isNeg) {
		if (origYIsNeg) {
			q = biAdd(q, bigOne);
		} else {
			q = biSubtract(q, bigOne);
		}
		y = biShiftRight(y, lambda);
		r = biSubtract(y, r);
	}

	if (r.digits[0] == 0 && biHighIndex(r) == 0) r.isNeg = false;

	return new Array(q, r);
}

function biDivide(x, y) {
	return biDivideModulo(x, y)[0];
}

function biModulo(x, y) {
	return biDivideModulo(x, y)[1];
}

function biMultiplyMod(x, y, m) {
	return biModulo(biMultiply(x, y), m);
}

function biPow(x, y) {
	var result = bigOne;
	var a = x;
	while (true) {
		if ((y & 1) != 0) result = biMultiply(result, a);
		y >>= 1;
		if (y == 0) break;
		a = biMultiply(a, a);
	}
	return result;
}

function biPowMod(x, y, m) {
	var result = bigOne;
	var a = x;
	var k = y;
	while (true) {
		if ((k.digits[0] & 1) != 0) result = biMultiplyMod(result, a, m);
		k = biShiftRight(k, 1);
		if (k.digits[0] == 0 && biHighIndex(k) == 0) break;
		a = biMultiplyMod(a, a, m);
	}
	return result;
}

function BarrettMu(m) {
	this.modulus = biCopy(m);
	this.k = biHighIndex(this.modulus) + 1;
	var b2k = new BigInt();
	b2k.digits[2 * this.k] = 1;
	this.mu = biDivide(b2k, this.modulus);
	this.bkplus1 = new BigInt();
	this.bkplus1.digits[this.k + 1] = 1;
	this.modulo = BarrettMu_modulo;
	this.multiplyMod = BarrettMu_multiplyMod;
	this.powMod = BarrettMu_powMod;
}

function BarrettMu_modulo(x) {
	var q1 = biDivideByRadixPower(x, this.k - 1);
	var q2 = biMultiply(q1, this.mu);
	var q3 = biDivideByRadixPower(q2, this.k + 1);
	var r1 = biModuloByRadixPower(x, this.k + 1);
	var r2term = biMultiply(q3, this.modulus);
	var r2 = biModuloByRadixPower(r2term, this.k + 1);
	var r = biSubtract(r1, r2);
	if (r.isNeg) {
		r = biAdd(r, this.bkplus1);
	}
	var rgtem = biCompare(r, this.modulus) >= 0;
	while (rgtem) {
		r = biSubtract(r, this.modulus);
		rgtem = biCompare(r, this.modulus) >= 0;
	}
	return r;
}

function BarrettMu_multiplyMod(x, y) {
	var xy = biMultiply(x, y);
	return this.modulo(xy);
}

function BarrettMu_powMod(x, y) {
	var result = new BigInt();
	result.digits[0] = 1;
	while (true) {
		if ((y.digits[0] & 1) != 0) result = this.multiplyMod(result, x);
		y = biShiftRight(y, 1);
		if (y.digits[0] == 0 && biHighIndex(y) == 0) break;
		x = this.multiplyMod(x, x);
	}
	return result;
}
