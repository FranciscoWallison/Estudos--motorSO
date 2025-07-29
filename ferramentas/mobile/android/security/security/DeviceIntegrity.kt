// DeviceIntegrity.kt
package br.com.beneficiario.odontoprev.security

import android.content.Context
import android.content.pm.PackageManager
import android.content.pm.Signature
import android.os.Build
import android.os.Debug
import android.util.Base64
import java.io.BufferedReader
import java.io.File
import java.io.InputStreamReader
import java.net.InetSocketAddress
import java.net.Socket
import java.security.MessageDigest
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

object DeviceIntegrity {

    private val suspiciousPaths = listOf(
        "/system/app/Superuser.apk", "/system/app/Magisk.apk",
        "/sbin/su", "/system/bin/su", "/system/xbin/su",
        "/data/local/xbin/su", "/data/local/bin/su",
        "/system/sd/xbin/su", "/system/bin/failsafe/su",
        "/data/local/su", "/su/bin/su", "/system/bin/magisk"
    )

    private val knownRootPackages = listOf(
        "com.topjohnwu.magisk", "eu.chainfire.supersu",
        "com.koushikdutta.superuser", "com.noshufou.android.su"
    )

    private fun isDeviceRooted(ctx: Context): Boolean =
        suspiciousPaths.any { File(it).exists() } ||
        Build.TAGS?.contains("test-keys") == true  ||
        hasSuInPath() ||
        canMountRw()  ||
        hasKnownRootApp(ctx)

    private fun hasSuInPath() = runCatching {
        Runtime.getRuntime()
            .exec(arrayOf("/system/xbin/which", "su"))
            .inputStream
            .bufferedReader()
            .readLine() != null
    }.getOrDefault(false)

    private fun canMountRw() = runCatching {
        File("/proc/mounts").readLines().any { line ->
            val cols = line.split(" ")
            cols.size >= 4 &&
            (cols[1] == "/system" || cols[1] == "/vendor") &&
            cols[3].contains("rw")
        }
    }.getOrDefault(false)

    private fun hasKnownRootApp(ctx: Context) =
        knownRootPackages.any { pkg ->
            runCatching { ctx.packageManager.getPackageInfo(pkg, 0) }.isSuccess
        }

    private fun basicEmulatorHeuristics() =
        Build.FINGERPRINT.startsWith("generic") ||
        Build.FINGERPRINT.startsWith("unknown") ||
        Build.MODEL.contains("google_sdk", true) ||
        Build.MODEL.contains("Emulator", true) ||
        Build.MODEL.contains("Android SDK built for x86", true) ||
        Build.MANUFACTURER.contains("Genymotion", true) ||
        (Build.BRAND.startsWith("generic") && Build.DEVICE.startsWith("generic")) ||
        "google_sdk" == Build.PRODUCT ||
        Build.HARDWARE.contains("goldfish") ||
        Build.HARDWARE.contains("ranchu")

    private fun isQemuPropertySet() = runCatching {
        val sys = Class.forName("android.os.SystemProperties")
        val get = sys.getMethod("get", String::class.java)
        get.invoke(null, "ro.kernel.qemu") == "1"
    }.getOrDefault(false)

    private fun missingCoreSensors(ctx: Context): Boolean {
        val pm = ctx.packageManager
        val hasLight  = pm.hasSystemFeature(PackageManager.FEATURE_SENSOR_LIGHT)
        val hasAccel  = pm.hasSystemFeature(PackageManager.FEATURE_SENSOR_ACCELEROMETER)
        return !hasLight || !hasAccel
    }

    private suspend fun adbPortOpenAsync(): Boolean = withContext(Dispatchers.IO) {
        runCatching {
            Socket().use { s ->
                s.connect(InetSocketAddress("10.0.2.2", 5555), 200)
                true
            }
        }.getOrDefault(false)
    }

    suspend fun isProbablyEmulator(ctx: Context): Boolean =
        basicEmulatorHeuristics() ||
        isQemuPropertySet()       ||
        missingCoreSensors(ctx)   ||
        adbPortOpenAsync()

    /*──────── DEBUGGER ────────*/
    private fun isDebuggerAttached() =
        Debug.isDebuggerConnected() || Debug.waitingForDebugger()

    /*─────── APK SIGNATURE ───────*/

    @Suppress("DEPRECATION")
    fun isSignatureValid(ctx: Context): Boolean {
        val pm = ctx.packageManager

        // ─── Obtém o primeiro certificado ───
        val rawSig: ByteArray = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            val info = pm.getPackageInfo(
                ctx.packageName,
                PackageManager.GET_SIGNING_CERTIFICATES
            )

            val sigArray = info.signingInfo?.apkContentsSigners
            if (sigArray.isNullOrEmpty()) {
                return false        // early-return fora de expressão
            }
            sigArray[0].toByteArray()
        } else {
            val info = pm.getPackageInfo(
                ctx.packageName,
                PackageManager.GET_SIGNATURES
            )

            val sigArray: Array<Signature>? = info.signatures
            val first = sigArray?.firstOrNull() ?: return false
            first.toByteArray()
        }

        // ─── Calcula o SHA-256 e compara ───
        val hash = MessageDigest
            .getInstance("SHA-256")
            .digest(rawSig)

        val current = Base64.encodeToString(hash, Base64.NO_WRAP)
        return current == SecurityConstants.getExpectedSignature()
    }

    suspend fun isCompromised(ctx: Context): Boolean =
        isDebuggerAttached() ||
        isDeviceRooted(ctx)  ||
        isProbablyEmulator(ctx)
        // VALIDAR ASSINATURA 
        //!isSignatureValid(ctx)
}
