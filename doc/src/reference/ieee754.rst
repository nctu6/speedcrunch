.. meta::
   :description: Use SpeedCrunch's IEEE 754 functions to encode and decode binary floating-point values, round to standard formats and inspect rounding residuals.

IEEE-754 Functions
==================

.. versionadded:: 0.12

IEEE-754 is a standard for the representation of and computations with floating point
numbers in binary systems. It is widely used by floating point implementations in
CPUs. These functions implement encoding and decoding binary representations of floating point
numbers according to IEEE-754.

An IEEE-754 binary float consists of three parts: a sign bit, the exponent and the significand
(sometimes called the mantissa). From these parts, the value is then calculated using the
following formula: ``-1 ^ sign * 2 ^ (exponent - bias) * 1.significand``.  The standard defines
multiple binary formats of different sizes that all follow these rules, but differ in
the number of bits allocated for the exponent and significand. The bias for the default
formats is defined as ``bias = (2 ^ (exponent_bits - 1)) - 1``.
See `this article <float_article_>`_ for a more detailed introduction into the subject.

.. _float_article: http://chrishecker.com/images/f/fb/Gdmfp.pdf

The following binary float formats are defined by the standard:

=============  ==================  =============  ================
Name           Also known as       Exponent bits  Significand bits
=============  ==================  =============  ================
``binary16``   Half precision      5              10
``binary32``   Single precision    8              23
``binary64``   Double precision    11             52
``binary128``  Quad precision      15             112
=============  ==================  =============  ================

In many programming languages, the ``binary32`` format is available as ``float``
and ``binary64`` is available as ``double``.

These functions convert between bit patterns and SpeedCrunch's own number
format. They do not make SpeedCrunch use IEEE-754 arithmetic internally.
Infinity decodes as ``NaN``. Both signs of zero decode as ``0``, and NaN signs,
payloads and the distinction between quiet and signaling NaNs are not preserved.

Very small finite values can appear as ``0`` in fixed-point notation because
the displayed result is rounded. The stored value is still nonzero. Use
:func:`sci` or select scientific notation to see these values, for example::

    sci(ieee754_double_decode(0x0000000000000001))
    = 4.94065645841246544177e-324


.. function:: ieee754_encode(x; exponent_bits; significand_bits [; exponent_bias])

   Encode a floating point number into a IEEE-754 binary representation.

   :param x: The floating point value to encode.
   :param exponent_bits: The length of the exponent part, in bits.
   :param significand_bits: The length of the significand part, in bits.
   :param exponent_bias: The exponent bias to use. Derived from the length of the
                         exponent if not specified.

.. function:: ieee754_decode(x; exponent_bits; significand_bits [; exponent_bias])

   Calculate the value of an IEEE-754 binary float.

   :param x: The binary float to decode.
   :param exponent_bits: The length of the exponent part, in bits.
   :param significand_bits: The length of the significand part, in bits.
   :param exponent_bias: The exponent bias to use. Derived from the length of the
                         exponent if not specified.

.. function:: ieee754_half_encode(x)

   Encode ``x`` in the half-precision binary format.

.. function:: ieee754_half_decode(x)

   Decode the half-precision binary float ``x``.

.. function:: ieee754_single_encode(x)

   Encode ``x`` in the single-precision binary format.

.. function:: ieee754_single_decode(x)

   Decode the single-precision binary float ``x``.

.. function:: ieee754_double_encode(x)

   Encode ``x`` in the double-precision binary format.

.. function:: ieee754_double_decode(x)

   Decode the double-precision binary float ``x``.

.. function:: ieee754_quad_encode(x)

   Encode ``x`` in the quad-precision binary format.

.. function:: ieee754_quad_decode(x)

   Decode the quad-precision binary float ``x``.

.. function:: ieee754_round_half(x)

   Round ``x`` to the nearest representable half-precision value.

.. function:: ieee754_round_float(x)

   Round ``x`` to the nearest representable single-precision value.

.. function:: ieee754_round_double(x)

   Round ``x`` to the nearest representable double-precision value.

.. function:: ieee754_round_quad(x)

   Round ``x`` to the nearest representable quad-precision value.

For the IEEE helper functions, let ``rounded`` mean the corresponding
``ieee754_round_*`` result.

- ``ieee754_*_residual(x) = x - rounded``
- absolute error magnitude = ``abs(ieee754_*_residual(x))``
- ULP-scaled signed error = ``ieee754_*_residual(x) / ulp(x)``

That is, residual uses the ``x - rounded`` sign convention (not ``rounded - x``).

.. function:: ieee754_half_residual(x)

   Return ``x - ieee754_round_half(x)``.

.. function:: ieee754_float_residual(x)

   Return ``x - ieee754_round_float(x)``.

.. function:: ieee754_double_residual(x)

   Return ``x - ieee754_round_double(x)``.

.. function:: ieee754_quad_residual(x)

   Return ``x - ieee754_round_quad(x)``.

There are currently no built-in ``ieee754_*_abs_error`` or
``ieee754_*_ulp_error`` functions.
