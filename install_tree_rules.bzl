def _staged_install_tree_impl(ctx):
    out = ctx.actions.declare_directory(ctx.label.name + "_root")

    binary = ctx.executable.binary
    binary_path = binary.path
    binary_name = binary.basename

    script = """
set -euo pipefail
OUT="{out}"
rm -rf "$OUT"
mkdir -p "$OUT/bin"
install -m755 "{binary_path}" "$OUT/bin/{binary_name}"
""".format(
        out = out.path,
        binary_path = binary_path,
        binary_name = binary_name,
    )

    ctx.actions.run_shell(
        inputs = [binary],
        outputs = [out],
        command = script,
        progress_message = "Staging install tree for {}".format(ctx.label.name),
    )

    return [DefaultInfo(files = depset([out]))]

staged_install_tree = rule(
    implementation = _staged_install_tree_impl,
    attrs = {
        "binary": attr.label(
            executable = True,
            cfg = "target",
            mandatory = True,
        ),
    },
    doc = "Stages an install tree rooted at bin/ containing the provided binary.",
)
