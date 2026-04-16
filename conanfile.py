from conan import ConanFile
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.gnu import PkgConfigDeps


class IngotConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "doctest/2.4.11"

    def configure(self):
        # meson.build pins cpp_std=c++20; force the matching value here so
        # MesonToolchain's generated native file doesn't override it with the
        # detected profile default (e.g. gnu17 on clang-18).
        self.settings.compiler.cppstd = "20"

    def generate(self):
        tc = MesonToolchain(self)
        tc.project_options["wrap_mode"] = "default"
        tc.generate()
        deps = PkgConfigDeps(self)
        deps.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()
