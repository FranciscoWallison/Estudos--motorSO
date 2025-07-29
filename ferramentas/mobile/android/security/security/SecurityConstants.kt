// SecurityConstants.kt
package br.com.beneficiario.odontoprev.security
import android.util.Base64

object SecurityConstants {

    private val OBFUSCATED_SIGNATURE = byteArrayOf(
        0x37, 0x53, 0x11, 0x1D, 0x19, 0x15, 0x0E, 0x33,
        0x27, 0x2E, 0x27, 0x18, 0x24, 0x43, 0x2E, 0x32,
        0x13, 0x09, 0x19, 0x64, 0x17, 0x0F, 0x33, 0x74,
        0x09, 0x05, 0x20, 0x10, 0x2B, 0x2A, 0x02, 0x24,
        0x16, 0x25, 0x48, 0x2B, 0x13, 0x2C, 0x07, 0x0C,
        0x24, 0x78, 0x70, 0x59
    )
    private const val OBFUSCATION_KEY = "@d@ntO!"

    /**
     * Retorna a assinatura esperada após desofuscá-la.
     * Torna muito mais difícil para um atacante simplesmente encontrar e substituir a string.
     */
    fun getExpectedSignature(): String {
        val keyBytes = OBFUSCATION_KEY.toByteArray()
        val deobfuscated = ByteArray(OBFUSCATED_SIGNATURE.size)
        for (i in OBFUSCATED_SIGNATURE.indices) {
            deobfuscated[i] = (OBFUSCATED_SIGNATURE[i].toInt() xor keyBytes[i % keyBytes.size].toInt()).toByte()
        }
        return String(deobfuscated)
    }
}