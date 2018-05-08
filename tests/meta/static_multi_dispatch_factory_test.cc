/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include "gmock/gmock.h"

#include "kahypar/meta/abstract_factory.h"
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/registrar.h"
#include "kahypar/meta/static_multi_dispatch_factory.h"

using ::testing::Test;

namespace kahypar {
namespace meta {
// The policies
struct PrintChar : PolicyBase { };
struct PrintInt : PolicyBase { };
struct PrintSymbol : PolicyBase { };

// The concrete policy classes
struct PrintA : PrintChar { static std::string print() { return std::string("A"); }
};
struct PrintB : PrintChar { static std::string print() { return std::string("B"); }
};
struct Print1 : PrintInt { static std::string print() { return std::string("1"); }
};

struct Print2 : PrintInt { static std::string print() { return std::string("2"); }
};

struct PrintHash : PrintSymbol { static std::string print() { return std::string("#"); }
};

struct PrintDollar : PrintSymbol { static std::string print() { return std::string("$"); }
};

struct PrintAt : PrintSymbol { static std::string print() { return std::string("@"); }
};


/*!
 * The abstract product / interface used by all concrete products.
 *
 */
struct IPrinter {
 public:
  virtual std::string printChar() = 0;
  virtual std::string printInt() = 0;
  virtual std::string printSymbol() = 0;
  virtual ~IPrinter() = default;
};

/*!
 * The concrete product which uses three policy classes.
 *
 * \tparam PrintChar The policy class for printing a character
 * \tparam PrintInt The policy class for printing an integer
 * \tparam PrintSymbol The policy class for printing a symbol
 */
template <class PrintChar,
          class PrintInt,
          class PrintSymbol>
class Printer : public IPrinter {
 public:
  Printer(int a, int b, const std::string& test) {
    LOG << a;
    LOG << b;
    LOG << test;
  }
  std::string printChar() override { return PrintChar::print(); }
  std::string printInt() override { return PrintInt::print(); }
  std::string printSymbol() override { return PrintSymbol::print(); }
};

/*!
 * A concrete product that does not use any policies
 */
class Z99StarPrinter : public IPrinter {
 public:
  Z99StarPrinter(int a, int b, const std::string& test) {
    LOG << a;
    LOG << b;
    LOG << test;
  }
  std::string printChar() override { return std::string("Z"); }
  std::string printInt() override { return std::string("99"); }
  std::string printSymbol() override { return std::string("*"); }
};

// The policies and their corresponding policy classes
using PrintCharPolicy = Typelist<PrintA, PrintB>;
using PrintIntPolicy = Typelist<Print1, Print2>;
using PrintSymbolPolicy = Typelist<PrintHash, PrintDollar, PrintAt>;

// The complete policy list
using PolicyList = Typelist<PrintCharPolicy, PrintIntPolicy, PrintSymbolPolicy>;

enum class PrintingAlgorithms : std::uint8_t {
  Printer1,
  Printer2
};

enum class PrinterPolicyClasses : std::uint16_t {
  PrintA,
  PrintB,
  Print1,
  Print2,
  PrintHash,
  PrintDollar,
  PrintAt
};

TEST(AStaticMultiDispatchFactory, AllowsDynamicSelectionOfStaticPolicies) {
  // Create a registry for printer policy classes
  using PrinterPolicyRegistry = PolicyRegistry<PrinterPolicyClasses>;

  // Register the policy classes
  static Registrar<PrinterPolicyRegistry> reg_a(PrinterPolicyClasses::PrintA, new PrintA());
  static Registrar<PrinterPolicyRegistry> reg_b(PrinterPolicyClasses::PrintB, new PrintB());
  static Registrar<PrinterPolicyRegistry> reg_1(PrinterPolicyClasses::Print1, new Print1());
  static Registrar<PrinterPolicyRegistry> reg_2(PrinterPolicyClasses::Print2, new Print2());
  static Registrar<PrinterPolicyRegistry> reg_hash(PrinterPolicyClasses::PrintHash,
                                                   new PrintHash());
  static Registrar<PrinterPolicyRegistry> reg_dollar(PrinterPolicyClasses::PrintDollar,
                                                     new PrintDollar());
  static Registrar<PrinterPolicyRegistry> reg_at(PrinterPolicyClasses::PrintAt,
                                                 new PrintAt());

  // Configure the FactoryDispatcher:
  // We want to create instances of type Printer, which implements the IPrinter
  // interface. Printers should be instantiated using the policies defined above.
  using PrinterFactoryDispatcher = StaticMultiDispatchFactory<Printer,
                                                              IPrinter,
                                                              PolicyList>;

  // A default printer configuration
  struct PrinterConfiguration {
    PrinterPolicyClasses char_policy = PrinterPolicyClasses::PrintA;
    PrinterPolicyClasses int_policy = PrinterPolicyClasses::Print2;
    PrinterPolicyClasses symbol_policy = PrinterPolicyClasses::PrintAt;
  };

  // We use a PrinterFactory that identifies concrete Printers using the
  // PrintingAlgorithms enum class. The factory function returns pointers
  // to IPrinter instances. Furthermore it takes 4 arguments. The first
  // argument is the configuration which will be used by the dispatcher
  // to build the correct instantiation. The remaining arguments are the
  // arguments for Printer's constructor.
  using PrinterFactory = Factory<PrintingAlgorithms,
                                 IPrinter* (*)(const PrinterConfiguration&,  // Policy configuration
                                               int,  // Constructor arguments ...
                                               int,
                                               const std::string&)>;


  // Register the first printer using the configuration for dispatching.
  Registrar<PrinterFactory> reg1(PrintingAlgorithms::Printer1,
                                 [](const PrinterConfiguration& config,
                                    int a, int b, const std::string& s) {
                                 // Forward a,b and s to the constructor of Printer and
                                 // instantiate the correct policies.
                                 return PrinterFactoryDispatcher::create(
                                   std::forward_as_tuple(a, b, s),     // constructor arguments
                                   PrinterPolicyRegistry::getInstance().getPolicy(config.char_policy),
                                   PrinterPolicyRegistry::getInstance().getPolicy(config.int_policy),
                                   PrinterPolicyRegistry::getInstance().getPolicy(config.symbol_policy));
        });

  // Register a second printer that does not use the dispatcher.
  Registrar<PrinterFactory> reg2(PrintingAlgorithms::Printer2,
                                 [](const PrinterConfiguration&,
                                    int a, int, const std::string& s) -> IPrinter* {
                                 return new Z99StarPrinter(a, a, s);
        });

  // Get a Printer1 instance that uses the default policy classes.
  std::unique_ptr<IPrinter> default_printer = PrinterFactory::getInstance().createObject(
    PrintingAlgorithms::Printer1, PrinterConfiguration(), 22, 23, "Constructor Arguments");

  ASSERT_EQ(default_printer->printChar(), "A");
  ASSERT_EQ(default_printer->printInt(), "2");
  ASSERT_EQ(default_printer->printSymbol(), "@");

  // Different configuration
  PrinterConfiguration new_config;
  new_config.char_policy = PrinterPolicyClasses::PrintB;
  new_config.int_policy = PrinterPolicyClasses::Print1;
  new_config.symbol_policy = PrinterPolicyClasses::PrintHash;

  // Get a Printer1 instance that uses the policy classes.
  std::unique_ptr<IPrinter> b1hash_printer = PrinterFactory::getInstance().createObject(
    PrintingAlgorithms::Printer1, new_config, 1, 2, "Constructor Arguments");

  ASSERT_EQ(b1hash_printer->printChar(), "B");
  ASSERT_EQ(b1hash_printer->printInt(), "1");
  ASSERT_EQ(b1hash_printer->printSymbol(), "#");

  // Get a Z99StarPrinter that does not have any template parameters and does not use
  // the dispatcher.
  std::unique_ptr<IPrinter> z99star_printer = PrinterFactory::getInstance().createObject(
    PrintingAlgorithms::Printer2, PrinterConfiguration(), 42, 1234, "Constructor Arguments");

  ASSERT_EQ(z99star_printer->printChar(), "Z");
  ASSERT_EQ(z99star_printer->printInt(), "99");
  ASSERT_EQ(z99star_printer->printSymbol(), "*");
}
}  // namespace meta
}  // namespace kahypar
