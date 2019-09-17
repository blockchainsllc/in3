package in3;

/**
 * The Proof type indicating how much proof is required.
 */

public enum Proof {
	/** No Verification */
	none,
	/** Standard Verification of the important properties */
	standard,
	/** Full Verification including even uncles wich leads to higher payload */
	full
}
