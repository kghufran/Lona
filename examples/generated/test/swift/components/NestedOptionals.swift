// Generated by Lona Compiler 0.6.0

import UIKit
import Foundation

// MARK: - NestedOptionals

public class NestedOptionals: UIView {

  // MARK: Lifecycle

  public init(_ parameters: Parameters) {
    self.parameters = parameters

    super.init(frame: .zero)

    setUpViews()
    setUpConstraints()

    update()
  }

  public convenience init() {
    self.init(Parameters())
  }

  public required init?(coder aDecoder: NSCoder) {
    self.parameters = Parameters()

    super.init(coder: aDecoder)

    setUpViews()
    setUpConstraints()

    update()
  }

  // MARK: Public

  public var parameters: Parameters {
    didSet {
      if parameters != oldValue {
        update()
      }
    }
  }

  // MARK: Private

  private var optionalsView = Optionals()

  private func setUpViews() {
    addSubview(optionalsView)

    optionalsView.boolParam = nil
    optionalsView.stringParam = "Text"
  }

  private func setUpConstraints() {
    translatesAutoresizingMaskIntoConstraints = false
    optionalsView.translatesAutoresizingMaskIntoConstraints = false

    let optionalsViewTopAnchorConstraint = optionalsView.topAnchor.constraint(equalTo: topAnchor)
    let optionalsViewBottomAnchorConstraint = optionalsView.bottomAnchor.constraint(equalTo: bottomAnchor)
    let optionalsViewLeadingAnchorConstraint = optionalsView.leadingAnchor.constraint(equalTo: leadingAnchor)
    let optionalsViewTrailingAnchorConstraint = optionalsView.trailingAnchor.constraint(equalTo: trailingAnchor)

    NSLayoutConstraint.activate([
      optionalsViewTopAnchorConstraint,
      optionalsViewBottomAnchorConstraint,
      optionalsViewLeadingAnchorConstraint,
      optionalsViewTrailingAnchorConstraint
    ])
  }

  private func update() {}
}

// MARK: - Parameters

extension NestedOptionals {
  public struct Parameters: Equatable {
    public init() {}
  }
}

// MARK: - Model

extension NestedOptionals {
  public struct Model: LonaViewModel, Equatable {
    public var id: String?
    public var parameters: Parameters
    public var type: String {
      return "NestedOptionals"
    }

    public init(id: String? = nil, parameters: Parameters) {
      self.id = id
      self.parameters = parameters
    }

    public init(_ parameters: Parameters) {
      self.parameters = parameters
    }

    public init() {
      self.init(Parameters())
    }
  }
}
